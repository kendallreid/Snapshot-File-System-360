#ifndef BLOCK_OPERATION_H
#define BLOCK_OPERATION_H

#include <stdio.h>
#include <stdint.h>

 
int read_block(FILE *disk, int block_num, uint8_t *buffer);

int write_block(FILE *disk, int block_num, const uint8_t *buffer);

int alloc_block(FILE *disk);

int free_block(FILE *disk, int block_num);

#endif