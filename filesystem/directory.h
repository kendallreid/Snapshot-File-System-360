#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>

#define MAX_NAME_LENGTH 32

typedef struct {
    char name[MAX_NAME_LENGTH];
    uint16_t inode_index;
} DirEntry;

#endif