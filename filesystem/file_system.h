#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include "directory.h"
#include "inode.h"

// Holds all filesystem state
typedef struct
{
    // Disk file
    FILE *disk;

    // Inode table
    Inode inode_table[MAX_INODES];

    // Root directory
    DirEntry root_dir[MAX_DIR_ENTRIES];
    int root_dir_count;
} FileSystem;

// Formats disk
int mkfs(FileSystem *fs, const char *disk_name);

// Initialize the file structure
int fs_init(FileSystem *fs);

// Create a new file or directory
int fs_create(FileSystem *fs, const char *name, inode_type type);

// Converts filename into inode index
int fs_lookup(FileSystem *fs, const char *name);

// List all files/directories
int fs_ls(FileSystem *fs);

// Helper to free inode blocks
void free_inode_blocks(FileSystem *fs, int inode_index);

// Store data from user
int fs_write(FileSystem *fs, int inode_index, const void *data, int size);

// Find blocks in inode and read
int fs_read(FileSystem *fs, int inode_index, void *buffer, int size);

// Delete a file by name
int fs_delete(FileSystem *fs, const char *name);

// Get file size
int fs_get_size(FileSystem *fs, int inode_index);

#endif