#ifndef VCS_INTERNAL_H
#define VCS_INTERNAL_H

#include "file_system.h"
#include "block_operations.h"

#define VCS_MAX_PATH 256
#define VCS_MAX_TEXT 2048
#define VCS_MAX_ENTRIES 256
#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

extern FileSystem fs;

typedef struct {
	char name[MAX_NAME_LENGTH];
	int inode_index;
	inode_type type;
} VcsEntryInfo;

int vcs_path_join(char *out, size_t out_sz, const char *a, const char *b);
void vcs_make_commit_id(int n, char *out, size_t out_sz);



#endif