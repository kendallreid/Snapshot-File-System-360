#include <string.h>
#include <stdio.h>
#include "file_system.h"

#include <stdlib.h>

#include "directory.h"
#include "inode.h"
#include "block_operations.h"
#include "bitmap.h"
#include "shared_values.h"

// Disk file
FILE *disk = NULL;

// Inode table
Inode inode_table[MAX_INODES];

// Root directory
DirEntry root_dir[MAX_DIR_ENTRIES];
int root_dir_count = 0;

// Formats disk
int mkfs()
{
    // Create/reset disk file
    disk = fopen("disk.dat", "w+b");
    if (disk == NULL) return -1;

    // Expand file to full size
    fseek(disk, BLOCK_SIZE * TOTAL_BLOCKS - 1, SEEK_SET);
    fwrite("", 1, 1, disk);
    fflush(disk);

    // Zero out disk
    uint8_t zero[BLOCK_SIZE];
    memset(zero, 0, BLOCK_SIZE);

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        fseek(disk, i * BLOCK_SIZE, SEEK_SET);
        fwrite(zero, 1, BLOCK_SIZE, disk);
    }

    fflush(disk);
    return 0;
}

// Initialize the file structure
// Creates the root directory
int fs_init()
{
    // Open disk file
    disk = fopen("disk.dat", "r+b");

    if (disk == NULL) {
        // If no disk exists, create one via mkfs
        mkfs();
    }

    // Clear all inodes for fresh start
    for (int i = 0; i < MAX_INODES; ++i) {
        inode_table[i].used = 0;
        inode_table[i].block_count = 0;
        inode_table[i].file_size = 0;

        for (int j = 0; j < 16; j++) {
            inode_table[i].blocks[j] = 0;
        }
    }

    // Make root inode (top-level directory / )
    inode_table[0].used = 1;
    inode_table[0].type = DIR_TYPE;
    inode_table[0].file_size = 0;
    inode_table[0].block_count = 0;

    // Clear directory
    root_dir_count = 0;

    return 0;
}

// Create a new file or directory
int fs_create(const char *name, inode_type type)
{
    // Find first free inode
    int inode_index = -1;
    for (int i = 0; i < MAX_INODES; i++) {
        if (inode_table[i].used == 0) {
            inode_index = i;
            break;
        }
    }
    if (inode_index == -1) return -1; // No space

    // Init inode
    inode_table[inode_index].used = 1;
    inode_table[inode_index].type = type;
    inode_table[inode_index].file_size = 0;
    inode_table[inode_index].block_count = 0;

    // Clear block pointers
    for (int i = 0; i < 16; i++) {
        inode_table[inode_index].blocks[i] = 0;
    }

    // Add new file/directory to root
    strcpy(root_dir[root_dir_count].name, name);
    root_dir[root_dir_count].inode_index = inode_index;
    ++root_dir_count;

    return inode_index;
}

// Converts filename into inode index
int fs_lookup(const char *name)
{
    if (name == NULL) return -1;

    for (int i = 0; i < root_dir_count; i++) {
        if (strcmp(root_dir[i].name, name) == 0)
            return root_dir[i].inode_index;
    }

    return -1; // Not found
}

// List all files/directories in current directory
int fs_ls()
{
    if (root_dir_count == 0)
    {
        printf("Empty directory\n");
        return 0;
    }

    // Print all entries
    for (int i = 0; i < root_dir_count; ++i)
    {
        int inode_index = root_dir[i].inode_index;

        printf("%s ", root_dir[i].name);

        if (inode_table[inode_index].type == DIR_TYPE)
            printf("[DIR] ");
        else
            printf("[FILE] ");

        printf("size: %u bytes\n", inode_table[inode_index].file_size);
    }
    return 0;
}

// Helper to free inode blocks
void free_inode_blocks(int inode_index)
{
    for (int i = 0; i < inode_table[inode_index].block_count; i++)
    {
        free_block(disk, inode_table[inode_index].blocks[i]);
        inode_table[inode_index].blocks[i] = 0;
    }

    inode_table[inode_index].block_count = 0;
    inode_table[inode_index].file_size = 0;
}

// Store data from user
int fs_write(int inode_index, const void *data, int size)
{
    // Check if valid inode index and use
    if (inode_table[inode_index].type == DIR_TYPE)
    {
        printf("Error: cannot write to a directory\n");
        return -1;
    }
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;
    if (inode_table[inode_index].used == 0)
        return -1;
    if (size <= 0)
        return -1;
    if (size > 16 * BLOCK_SIZE)
        return -1;

    // Load bitmap into RAM from disk - tracks which blocks are free
    uint8_t bitmap_buffer[BLOCK_SIZE];
    if (read_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0)
        return -1;

    // Free old blocks
    free_inode_blocks(inode_index);

    // Prepare as input data
    uint8_t *byte_data = (uint8_t *)data;

    // Blocks needed - ceiling division
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Allocate + write each block
    for (int i = 0; i < blocks_needed; i++)
    {
        // Allocate a block from bitmap
        int block = bitmap_allocate(bitmap_buffer, TOTAL_BLOCKS);
        if (block == -1)
            return -1;

        // Prepare block buffer
        uint8_t buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        // Offset in file and bytes to copy
        int offset = i * BLOCK_SIZE;
        int chunk = BLOCK_SIZE;

        if (offset + chunk > size)
            chunk = size - offset;

        // Copy file data into a block
        memcpy(buffer, byte_data + offset, chunk);

        // Write block data to disk
        if (write_block(disk, block, buffer) != 0)
            return -1;

        // Store in inode
        inode_table[inode_index].blocks[i] = block;
        inode_table[inode_index].block_count++;
    }

    // Save updated bitmap back to disk
    if (write_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0)
        return -1;

    return 0;
}

// Find blocks in inode and read
int fs_read(int inode_index, void *buffer, int size)
{
    if (inode_table[inode_index].type == DIR_TYPE)
    {
        printf("Error: cannot read a directory\n");
        return -1;
    }
    // Check valid inode
    if (inode_index < 0 || inode_index >= MAX_INODES)
        return -1;

    if (inode_table[inode_index].used == 0)
        return -1;

    // Actual bytes available in file
    int file_size = inode_table[inode_index].file_size;

    // Do not read past end of file
    if (size > file_size)
        size = file_size;

    uint8_t *out = (uint8_t *)buffer;

    int bytes_read = 0;

    // Read each block listed in inode
    for (int i = 0; i < inode_table[inode_index].block_count; i++)
    {
        int block = inode_table[inode_index].blocks[i];

        uint8_t block_buffer[BLOCK_SIZE];

        if (read_block(disk, block, block_buffer) != 0)
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
int fs_delete(const char *name)
{
    if (name == NULL)
        return -1;
    int dir_index = -1;

    // Find entry
    for (int i = 0; i < root_dir_count; ++i)
    {
        if (strcmp(root_dir[i].name, name) == 0)
        {
            dir_index = i;
            break;
        }
    }

    // File not found
    if (dir_index == -1)
        return -1;

    // Clear out file and all associated blocks and inode
    int inode_index = root_dir[dir_index].inode_index;

    if (inode_table[inode_index].type == DIR_TYPE)
    {
        // in current system: safe to delete
        // in real FS: check if empty first
    }

    free_inode_blocks(inode_index);
    inode_table[inode_index].used = 0;
    inode_table[inode_index].type = FILE_TYPE;

    // Remove entry by shifting left so no gaps
    for (int i = dir_index; i < root_dir_count -1; ++i)
    {
        root_dir[i] = root_dir[i + 1];
    }
    --root_dir_count;
    return 0;
}