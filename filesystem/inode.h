#ifndef INODE_H
#define INODE_H

#include <stdint.h>

#define FILE_TYPE 0
#define DIR_TYPE  1

typedef uint8_t inode_type;

typedef struct {
    uint8_t used; // Availability 0 = free, 1 = used
    inode_type type; // File or directory
    uint32_t file_size; // Actual size of file in bytes
    uint8_t block_count; // Max 16
    uint16_t blocks[16]; // Block indexes (0-1023)
}Inode;


#endif