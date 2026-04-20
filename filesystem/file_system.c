#include <string.h>
#include <stdio.h>
#include "file_system.h"

#include <stdlib.h>

#include "directory.h"
#include "inode.h"
#include "../filestructures/block_operations.h"
#include "../filestructures/bitmap.h"
#include "../shared_values.h"

// Formats disk
int mkfs(FileSystem *fs, const char *disk_name)
{
    fs->disk = fopen(disk_name, "w+b");
    if (!fs->disk) return -1;

    uint8_t zero[BLOCK_SIZE];
    memset(zero, 0, BLOCK_SIZE);

    // Write empty disk
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        fseek(fs->disk, i * BLOCK_SIZE, SEEK_SET);
        fwrite(zero, 1, BLOCK_SIZE, fs->disk);
    }

    // Initialize bitmap
    uint8_t bitmap[BLOCK_SIZE];
    memset(bitmap, 0, BLOCK_SIZE);

    // mark bitmap block as used
    bitmap_set(bitmap, DATA_BITMAP_BLOCK);

    fseek(fs->disk, DATA_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fs->disk);

    fflush(fs->disk);
    return 0;
}

// Initialize the file structure
// Creates the root directory
int fs_init(FileSystem *fs)
{
    // Open disk file
    fs->disk = fopen("disk.dat", "r+b");

    if (fs->disk == NULL) {
        // If no disk exists, create one via mkfs
        mkfs(fs, "disk.dat");
    }

    // Clear all inodes for fresh start
    for (int i = 0; i < MAX_INODES; ++i) {
        fs->inode_table[i].used = 0;
        fs->inode_table[i].block_count = 0;
        fs->inode_table[i].file_size = 0;

        for (int j = 0; j < 16; j++) {
            fs->inode_table[i].blocks[j] = 0;
        }
    }

    // Make root inode (top-level directory / )
    fs->inode_table[0].used = 1;
    fs->inode_table[0].type = DIR_TYPE;
    fs->inode_table[0].file_size = 0;
    fs->inode_table[0].block_count = 0;

    // Clear directory
    fs->root_dir_count = 0;

    return 0;
}

// Create a new file or directory
int fs_create(FileSystem *fs, const char *name, inode_type type)
{
    // Find first free inode
    int inode_index = -1;

    for (int i = 0; i < MAX_INODES; i++) {
        if (fs->inode_table[i].used == 0) {
            inode_index = i;
            break;
        }
    }

    if (inode_index == -1) return -1; // No space

    // Init inode
    fs->inode_table[inode_index].used = 1;
    fs->inode_table[inode_index].type = type;
    fs->inode_table[inode_index].file_size = 0;
    fs->inode_table[inode_index].block_count = 0;

    // Clear block pointers
    for (int i = 0; i < 16; i++)
        fs->inode_table[inode_index].blocks[i] = 0;

    // Add new file/directory to root
    strcpy(fs->root_dir[fs->root_dir_count].name, name);
    fs->root_dir[fs->root_dir_count].inode_index = inode_index;
    fs->root_dir_count++;

    return inode_index;
}

// Converts filename into inode index
int fs_lookup(FileSystem *fs, const char *name)
{
    if (name == NULL) return -1;

    for (int i = 0; i < fs->root_dir_count; i++) {
        if (strcmp(fs->root_dir[i].name, name) == 0)
            return fs->root_dir[i].inode_index;
    }

    return -1; // Not found
}

// List all files/directories in current directory
int fs_ls(FileSystem *fs)
{
    if (fs->root_dir_count == 0)
    {
        printf("Empty directory\n");
        return 0;
    }

    // Print all entries
    for (int i = 0; i < fs->root_dir_count; ++i)
    {
        int inode_index = fs->root_dir[i].inode_index;

        printf("%s ", fs->root_dir[i].name);

        if (fs->inode_table[inode_index].type == DIR_TYPE)
            printf("[DIR] ");
        else
            printf("[FILE] ");

        printf("size: %u bytes\n", fs->inode_table[inode_index].file_size);
    }

    return 0;
}

// Helper to free inode blocks
void free_inode_blocks(FileSystem *fs, int inode_index)
{
    for (int i = 0; i < fs->inode_table[inode_index].block_count; i++)
    {
        free_block(fs->disk, fs->inode_table[inode_index].blocks[i]);
        fs->inode_table[inode_index].blocks[i] = 0;
    }

    fs->inode_table[inode_index].block_count = 0;
    fs->inode_table[inode_index].file_size = 0;
}

// Store data from user
int fs_write(FileSystem *fs, int inode_index, const void *data, int size)
{
    // Check if valid inode index and use
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    if (fs->inode_table[inode_index].used == 0)
        return -1;

    if (fs->inode_table[inode_index].type == DIR_TYPE)
    {
        printf("Error: cannot write to a directory\n");
        return -1;
    }

    if (size <= 0)
        return -1;

    if (size > 16 * BLOCK_SIZE)
        return -1;

    // Save old block list in case write fails
    int old_blocks[16];
    int old_count = fs->inode_table[inode_index].block_count;

    for (int i = 0; i < old_count; i++)
        old_blocks[i] = fs->inode_table[inode_index].blocks[i];

    // Prepare input data as byte array
    const uint8_t *byte_data = (const uint8_t *)data;

    // Blocks needed - ceiling division for correct number of blocks
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Temporary new block list
    int new_blocks[16];

    // Allocate + write each block
    for (int i = 0; i < blocks_needed; i++)
    {
        // Allocate a new free block
        int block = alloc_block(fs->disk);

        if (block == -1)
        {
            printf("Error: no free blocks available\n");

            // free any newly allocated blocks
            for (int j = 0; j < i; j++)
                free_block(fs->disk, new_blocks[j]);

            return -1;
        }

        // Prepare block buffer
        uint8_t buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        // Calculate offset in input data
        int offset = i * BLOCK_SIZE;
        int chunk = BLOCK_SIZE;

        // Handle last partial block
        if (offset + chunk > size)
            chunk = size - offset;

        // Copy file data into block buffer
        memcpy(buffer, byte_data + offset, chunk);

        // Write block to disk
        if (write_block(fs->disk, block, buffer) != 0)
        {
            free_block(fs->disk, block);

            for (int j = 0; j < i; j++)
                free_block(fs->disk, new_blocks[j]);

            return -1;
        }

        new_blocks[i] = block;
    }

    // All writes successful
    // Now free old blocks
    for (int i = 0; i < old_count; i++)
        free_block(fs->disk, old_blocks[i]);

    // Update inode to new blocks
    for (int i = 0; i < blocks_needed; i++)
        fs->inode_table[inode_index].blocks[i] = new_blocks[i];

    for (int i = blocks_needed; i < 16; i++)
        fs->inode_table[inode_index].blocks[i] = 0;

    fs->inode_table[inode_index].block_count = blocks_needed;
    fs->inode_table[inode_index].file_size = size;

    return 0;
}

// Find blocks in inode and read
int fs_read(FileSystem *fs, int inode_index, void *buffer, int size)
{
    // Check valid inode
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    if (fs->inode_table[inode_index].used == 0)
        return -1;

    if (fs->inode_table[inode_index].type == DIR_TYPE)
    {
        printf("Error: cannot read a directory\n");
        return -1;
    }

    // Actual bytes available in file
    int file_size = fs->inode_table[inode_index].file_size;

    // Do not read past end of file
    if (size > file_size)
        size = file_size;

    uint8_t *out = (uint8_t *)buffer;

    int bytes_read = 0;

    // Read each block listed in inode
    for (int i = 0; i < fs->inode_table[inode_index].block_count; i++)
    {
        int block = fs->inode_table[inode_index].blocks[i];

        uint8_t block_buffer[BLOCK_SIZE];

        if (read_block(fs->disk, block, block_buffer) != 0)
            return -1;

        int chunk = BLOCK_SIZE;

        if (bytes_read + chunk > size)
            chunk = size - bytes_read;

        memcpy(out + bytes_read, block_buffer, chunk);

        bytes_read += chunk;

        if (bytes_read >= size)
            break;
    }

    return bytes_read;
}

// Delete a file by name
int fs_delete(FileSystem *fs, const char *name)
{
    if (name == NULL)
        return -1;

    int dir_index = -1;

    // Find entry
    for (int i = 0; i < fs->root_dir_count; ++i)
    {
        if (strcmp(fs->root_dir[i].name, name) == 0)
        {
            dir_index = i;
            break;
        }
    }

    // File not found
    if (dir_index == -1)
        return -1;

    // Clear out file and all associated blocks and inode
    int inode_index = fs->root_dir[dir_index].inode_index;

    if (fs->inode_table[inode_index].type == DIR_TYPE)
    {
        // in current system: safe to delete
        // in real FS: check if empty first
    }

    free_inode_blocks(fs, inode_index);

    fs->inode_table[inode_index].used = 0;
    fs->inode_table[inode_index].type = FILE_TYPE;

    // Remove entry by shifting left so no gaps
    for (int i = dir_index; i < fs->root_dir_count - 1; ++i)
        fs->root_dir[i] = fs->root_dir[i + 1];

    fs->root_dir_count--;

    return 0;
}

// Gets file size safely
int fs_get_size(FileSystem *fs, int inode_index)
{
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    if (fs->inode_table[inode_index].used == 0)
        return -1;

    return fs->inode_table[inode_index].file_size;
}