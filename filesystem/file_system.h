#ifndef FS_H
#define FS_H

#include "inode.h"

int mkfs(const char *disk_name);
int fs_init();
int fs_create(const char *name, inode_type type);
int fs_lookup(const char *name);
int fs_ls();
void free_inode_blocks(int inode_index);
int fs_write(int inode_index, const void *data, int size);
int fs_read(int inode_index, void *buffer, int size);
int fs_delete(const char *name);

#endif