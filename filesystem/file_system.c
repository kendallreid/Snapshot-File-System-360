#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_system.h"
#include "directory.h"
#include "inode.h"
#include "../filestructures/block_operations.h"
#include "../filestructures/bitmap.h"
#include "../shared_values.h"

#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

// Creates a brand new empty disk file
int mkfs(FileSystem *fs, const char *disk_name)
{
    // Open disk for binary read/write, truncate old contents
    fs->disk = fopen(disk_name, "w+b");
    if (!fs->disk) return -1;

    // Buffer of zeros used to clear every block
    uint8_t zero[BLOCK_SIZE];
    memset(zero, 0, BLOCK_SIZE);

    // Write zeros across entire disk
    for (int i = 0; i < TOTAL_BLOCKS; i++)
    {
        fseek(fs->disk, i * BLOCK_SIZE, SEEK_SET);
        fwrite(zero, 1, BLOCK_SIZE, fs->disk);
    }

    // Build empty bitmap block
    uint8_t bitmap[BLOCK_SIZE];
    memset(bitmap, 0, BLOCK_SIZE);

    // Mark bitmap block itself as used
    bitmap_set(bitmap, DATA_BITMAP_BLOCK);

    // Save bitmap to disk
    fseek(fs->disk, DATA_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fs->disk);

    fflush(fs->disk);
    return 0;
}

// Loads existing disk or creates new one
int fs_init(FileSystem *fs)
{
    // Try opening previous disk
    fs->disk = fopen("disk.dat", "r+b");

    // If none exists, create one
    if (!fs->disk)
        mkfs(fs, "disk.dat");

    // Reset every inode entry in memory
    for (int i = 0; i < MAX_INODES; i++)
    {
        fs->inode_table[i].used = 0;
        fs->inode_table[i].type = FILE_TYPE;
        fs->inode_table[i].file_size = 0;
        fs->inode_table[i].block_count = 0;
        fs->inode_table[i].parent_inode = 0;

        // Clear block pointers
        for (int j = 0; j < MAX_BLOCKS_PER_INODE; j++)
            fs->inode_table[i].blocks[j] = 0;
    }

    // Create root directory inode
    fs->inode_table[0].used = 1;
    fs->inode_table[0].type = DIR_TYPE;
    fs->inode_table[0].parent_inode = 0;

    // Start current directory at root
    fs->cwd_inode = 0;

    return 0;
}

// Searches a directory for a named entry
static int find_in_dir(FileSystem *fs, int dir_inode, const char *name, int *out_inode)
{
    Inode *dir = &fs->inode_table[dir_inode];

    // Check every data block in directory
    for (int b = 0; b < dir->block_count; b++)
    {
        uint8_t buffer[BLOCK_SIZE];
        read_block(fs->disk, dir->blocks[b], buffer);

        DirEntry *entries = (DirEntry *)buffer;

        // Scan each directory slot
        for (int i = 0; i < ENTRIES_PER_BLOCK; i++)
        {
            // Match valid name
            if (entries[i].name[0] &&
                strcmp(entries[i].name, name) == 0)
            {
                *out_inode = entries[i].inode_index;
                return 1;
            }
        }
    }

    return 0;
}

// Converts path string into inode number
int fs_lookup_path(FileSystem *fs, const char *path)
{
    // Empty path means current directory
    if (!path || path[0] == '\0')
        return fs->cwd_inode;

    // Absolute starts at root, relative starts at cwd
    int current = (path[0] == '/') ? 0 : fs->cwd_inode;

    char temp[256];
    strcpy(temp, path);

    // Split path by /
    char *token = strtok(temp, "/");

    while (token)
    {
        // Ignore current directory token
        if (strcmp(token, ".") == 0)
        {
            token = strtok(NULL, "/");
            continue;
        }

        // Move to parent directory
        if (strcmp(token, "..") == 0)
        {
            if (current != 0)
                current = fs->inode_table[current].parent_inode;

            token = strtok(NULL, "/");
            continue;
        }

        // Must currently be inside a directory
        if (fs->inode_table[current].type != DIR_TYPE)
            return -1;

        int next;

        // Find next path piece in current dir
        if (!find_in_dir(fs, current, token, &next))
            return -1;

        current = next;
        token = strtok(NULL, "/");
    }

    return current;
}

// Inserts a new file/dir entry into a directory
static int insert_into_dir(FileSystem *fs, int dir_inode, const char *name, int inode_index)
{
    Inode *dir = &fs->inode_table[dir_inode];

    // If directory has no block yet, allocate first one
    if (dir->block_count == 0)
    {
        int block = alloc_block(fs->disk);
        if (block == -1) return -1;

        dir->blocks[0] = block;
        dir->block_count = 1;

        uint8_t empty[BLOCK_SIZE];
        memset(empty, 0, BLOCK_SIZE);
        write_block(fs->disk, block, empty);
    }

    // Search existing blocks for free slot
    for (int b = 0; b < dir->block_count; b++)
    {
        uint8_t buffer[BLOCK_SIZE];
        read_block(fs->disk, dir->blocks[b], buffer);

        DirEntry *entries = (DirEntry *)buffer;

        for (int i = 0; i < ENTRIES_PER_BLOCK; i++)
        {
            // Empty slot found
            if (entries[i].name[0] == '\0')
            {
                strcpy(entries[i].name, name);
                entries[i].inode_index = inode_index;

                write_block(fs->disk, dir->blocks[b], buffer);
                return 0;
            }
        }
    }

    // No space left in directory
    if (dir->block_count >= MAX_BLOCKS_PER_INODE)
        return -1;

    // Add another directory block
    int block = alloc_block(fs->disk);
    if (block == -1) return -1;

    dir->blocks[dir->block_count++] = block;

    uint8_t empty[BLOCK_SIZE];
    memset(empty, 0, BLOCK_SIZE);

    // Put new entry in first slot
    DirEntry *entries = (DirEntry *)empty;
    strcpy(entries[0].name, name);
    entries[0].inode_index = inode_index;

    write_block(fs->disk, block, empty);

    return 0;
}

// Creates a file or directory inode + parent entry
int fs_create(FileSystem *fs, const char *path, inode_type type)
{
    char full[256];
    strcpy(full, path);

    char parent[256];
    char name[256];

    // Find final slash
    char *last = strrchr(full, '/');

    // Relative file in cwd
    if (!last)
    {
        strcpy(name, full);
        strcpy(parent, "");
    }
    else
    {
        // Text after slash is object name
        strcpy(name, last + 1);

        // Root child like /a
        if (last == full)
        {
            strcpy(parent, "/");
        }
        else
        {
            // Split into parent + child
            *last = '\0';
            strcpy(parent, full);
        }
    }

    int parent_inode;

    // Empty parent means cwd
    if (parent[0] == '\0')
        parent_inode = fs->cwd_inode;
    else
        parent_inode = fs_lookup_path(fs, parent);

    if (parent_inode == -1)
        return -1;

    int inode_index = -1;

    // Find unused inode slot
    for (int i = 0; i < MAX_INODES; i++)
    {
        if (!fs->inode_table[i].used)
        {
            inode_index = i;
            break;
        }
    }

    if (inode_index == -1)
        return -1;

    // Initialize inode metadata
    fs->inode_table[inode_index].used = 1;
    fs->inode_table[inode_index].type = type;
    fs->inode_table[inode_index].file_size = 0;
    fs->inode_table[inode_index].block_count = 0;
    fs->inode_table[inode_index].parent_inode = parent_inode;

    memset(fs->inode_table[inode_index].blocks, 0,
           sizeof(fs->inode_table[inode_index].blocks));

    // Add name into parent directory
    return insert_into_dir(fs, parent_inode, name, inode_index);
}
// Lists contents of a directory
int fs_ls(FileSystem *fs, const char *path)
{
    // Resolve path to inode
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
        return -1;

    // Must be a directory
    if (fs->inode_table[inode].type != DIR_TYPE)
        return -1;

    Inode *dir = &fs->inode_table[inode];

    // Read every directory block
    for (int i = 0; i < dir->block_count; i++)
    {
        uint8_t buffer[BLOCK_SIZE];
        read_block(fs->disk, dir->blocks[i], buffer);

        DirEntry *entries = (DirEntry *)buffer;

        // Print each used entry
        for (int j = 0; j < ENTRIES_PER_BLOCK; j++)
        {
            if (entries[j].name[0])
            {
                printf("%s [%s]\n",
                    entries[j].name,
                    fs->inode_table[entries[j].inode_index].type == DIR_TYPE
                        ? "DIR" : "FILE");
            }
        }
    }

    return 0;
}

// Removes matching entry from a directory
static int remove_from_dir(FileSystem *fs, int dir_inode, int target_inode)
{
    Inode *dir = &fs->inode_table[dir_inode];

    // Search every block
    for (int b = 0; b < dir->block_count; b++)
    {
        uint8_t buffer[BLOCK_SIZE];
        read_block(fs->disk, dir->blocks[b], buffer);

        DirEntry *entries = (DirEntry *)buffer;

        // Search each slot
        for (int i = 0; i < ENTRIES_PER_BLOCK; i++)
        {
            if (entries[i].inode_index == target_inode &&
                entries[i].name[0])
            {
                // Clear directory entry
                memset(&entries[i], 0, sizeof(DirEntry));
                write_block(fs->disk, dir->blocks[b], buffer);
                return 0;
            }
        }
    }

    return -1;
}

// Deletes file or directory
int fs_delete(FileSystem *fs, const char *path)
{
    // Find inode
    int inode = fs_lookup_path(fs, path);

    // Protect root or invalid path
    if (inode <= 0)
        return -1;

    int parent = fs->inode_table[inode].parent_inode;

    // Remove from parent listing
    if (remove_from_dir(fs, parent, inode) == -1)
        return -1;

    // Free file blocks
    free_inode_blocks(fs, inode);

    // Mark inode unused
    fs->inode_table[inode].used = 0;

    return 0;
}

// Returns file size
int fs_get_size(FileSystem *fs, int inode_index)
{
    // Validate inode range
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    // Must be active inode
    if (!fs->inode_table[inode_index].used)
        return -1;

    return fs->inode_table[inode_index].file_size;
}

// Changes current directory
int fs_cd(FileSystem *fs, const char *path)
{
    // Find destination
    int inode = fs_lookup_path(fs, path);

    if (inode == -1)
        return -1;

    // Must be directory
    if (fs->inode_table[inode].type != DIR_TYPE)
        return -1;

    // Update cwd
    fs->cwd_inode = inode;

    return 0;
}

// Frees all data blocks owned by inode
void free_inode_blocks(FileSystem *fs, int inode_index)
{
    Inode *node = &fs->inode_table[inode_index];

    // Release each block
    for (int i = 0; i < node->block_count; i++)
    {
        free_block(fs->disk, node->blocks[i]);
        node->blocks[i] = 0;
    }

    // Reset metadata
    node->block_count = 0;
    node->file_size = 0;
}

// Writes data into file
int fs_write(FileSystem *fs, int inode_index, const void *data, int size)
{
    // Validate inode
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    Inode *node = &fs->inode_table[inode_index];

    // Must be used regular file
    if (!node->used || node->type == DIR_TYPE)
        return -1;

    // Remove old contents first
    free_inode_blocks(fs, inode_index);

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    const uint8_t *src = (const uint8_t *)data;

    // Allocate and write blocks
    for (int i = 0; i < blocks_needed; i++)
    {
        int block = alloc_block(fs->disk);
        if (block == -1)
            return -1;

        uint8_t buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        int offset = i * BLOCK_SIZE;
        int chunk = (offset + BLOCK_SIZE > size)
            ? (size - offset)
            : BLOCK_SIZE;

        memcpy(buffer, src + offset, chunk);

        write_block(fs->disk, block, buffer);

        node->blocks[i] = block;
    }

    // Save metadata
    node->block_count = blocks_needed;
    node->file_size = size;

    return 0;
}

// Reads file contents into buffer
int fs_read(FileSystem *fs, int inode_index, void *buffer, int size)
{
    // Validate inode
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    Inode *node = &fs->inode_table[inode_index];

    if (!node->used)
        return -1;

    // Prevent reading past EOF
    if (size > node->file_size)
        size = node->file_size;

    uint8_t *out = (uint8_t *)buffer;
    int read_bytes = 0;

    // Read each block
    for (int i = 0; i < node->block_count; i++)
    {
        uint8_t block_buffer[BLOCK_SIZE];
        read_block(fs->disk, node->blocks[i], block_buffer);

        int chunk = BLOCK_SIZE;

        if (read_bytes + chunk > size)
            chunk = size - read_bytes;

        memcpy(out + read_bytes, block_buffer, chunk);
        read_bytes += chunk;

        if (read_bytes >= size)
            break;
    }

    return read_bytes;
}