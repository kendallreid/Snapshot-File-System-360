#ifndef VCS_HELPERS_H
#define VCS_HELPERS_H

#include "file_system.h"

extern FileSystem fs;

int vcs_path_join(char *out, size_t n, const char *a, const char *b);
int vcs_ensure_exists(const char *path, inode_type type);
int vcs_write_text(const char *path, const char *text);
int vcs_read_text(const char *path, char *out, int n);
void vcs_make_commit_id(int n, char *out, size_t size);

#endif