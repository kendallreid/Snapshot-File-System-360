#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include "directory.h"
#include "inode.h"

// =============================
// FILE SYSTEM STATE
// =============================
typedef struct
{
    // Disk file
    FILE *disk;

    // Inode table (ALL files + directories)
    Inode inode_table[MAX_INODES];

    // =============================
    // CURRENT WORKING DIRECTORY
    // =============================
    int cwd_inode;

} FileSystem;

// Format disk (initialize empty filesystem)
int mkfs(FileSystem *fs, const char *disk_name);

// Initialize filesystem (load or create)
int fs_init(FileSystem *fs);

// Create file or directory using full or relative path
// Example: "/a/b/c.txt" OR "file.txt"
int fs_create(FileSystem *fs, const char *path, inode_type type);

// Resolve full or relative path to inode index
// Supports: "/", "/a/b", "a/b", ".", ".."
int fs_lookup_path(FileSystem *fs, const char *path);

// List directory contents at a given path or cwd if NULL
int fs_ls(FileSystem *fs, const char *path);

// Delete file or directory by path (absolute or relative)
int fs_delete(FileSystem *fs, const char *path);

// Change current working directory
// Supports: cd /a/b, cd .., cd ., cd a/b
int fs_cd(FileSystem *fs, const char *path);

// Write data to file inode
int fs_write(FileSystem *fs, int inode_index, const void *data, int size);

// Read data from file inode
int fs_read(FileSystem *fs, int inode_index, void *buffer, int size);

// Get file size
int fs_get_size(FileSystem *fs, int inode_index);

// Free all blocks belonging to an inode
void free_inode_blocks(FileSystem *fs, int inode_index);

#endif