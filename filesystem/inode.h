#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include "shared_values.h"


#define FILE_TYPE 0
#define DIR_TYPE  1

typedef uint8_t inode_type;

// Describes file or directory
typedef struct {
    uint8_t used; // Availability 0 = free, 1 = used
    inode_type type; // File or directory
    uint32_t file_size; // Actual size of file in bytes or (# entries * bytes)
    uint8_t block_count; // Max 16
    uint16_t blocks[MAX_BLOCKS_PER_INODE]; // Block indexes (0-1023)
}Inode;


#endif