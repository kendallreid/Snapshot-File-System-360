#include "block_operations.h"
#include "../shared_values.h"
#include "bitmap.h"

// reads one full block from the disk image into the buffer.
int read_block(FILE *disk, int block_num, uint8_t *buffer)
{
    // validate imputs
    if (disk == NULL || buffer == NULL || block_num < 0) {
        return -1;
    }
    // compute the byte offset where this block begins.
    long offset = (long)block_num * BLOCK_SIZE;

    // move the file pointer to the start of the requested block
    if (fseek(disk, offset, SEEK_SET) != 0) {
        return -1;
    }
    // read exactly one block into the buffer.
    // since each item is 1 byte
    // we expect BLOCK_SIZE items.

    //returns how many items were successfully read.
    size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, disk);
    if (bytes_read != BLOCK_SIZE) {
        return -1;
    }
    return 0;
}

// writes one full block from the buffer into the disk image.
int write_block(FILE *disk, int block_num, const uint8_t *buffer)
{
    // validate imputs make
    if (disk == NULL || buffer == NULL || block_num < 0) {
        return -1;
    }

    // compute the byte offset where this block begins
    long offset = (long)block_num * BLOCK_SIZE;

    // move the file pointer to the start of the requested block.
    if (fseek(disk, offset, SEEK_SET) != 0) {
        return -1;
    }

    // write exactly one full block from the buffer into the file.
    size_t bytes_written = fwrite(buffer, 1, BLOCK_SIZE, disk);
    if (bytes_written != BLOCK_SIZE) {
        return -1;
    }

     // flush buffered output so the data is actually pushed to the file.
    if (fflush(disk) != 0) {
        return -1;
    }
    return 0;
}

// Allocates the first free block recorded in the data bitmap.
int alloc_block(FILE *disk)
{
    uint8_t bitmap_buffer[BLOCK_SIZE];

    // load the data block bitmap from its block in the disk image.
    if (read_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0) {
        return -1;
    }

    // ask the bitmap module to find and reserve the first free block bit.
    int allocated_index = bitmap_allocate(bitmap_buffer, TOTAL_BLOCKS);
    if (allocated_index == -1) {
        return -1;
    }

    // save the updated bitmap back to disk. 
    if (write_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0) {
        return -1;
    }
    return allocated_index;
}
  

// frees the given block number in the data block bitmap
int free_block(FILE *disk, int block_num)
{
    uint8_t bitmap_buffer[BLOCK_SIZE];

    if (block_num < 0 || block_num >= TOTAL_BLOCKS) {
        return -1;
    }

    // load the current bitmap block from disk.
    if (read_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0) {
        return -1;
    }

    // mark the requested block as free. 
    if (bitmap_free(bitmap_buffer, TOTAL_BLOCKS, block_num) != 0) {
        return -1;
    }

    //Save the updated bitmap back to disk. 
    if (write_block(disk, DATA_BITMAP_BLOCK, bitmap_buffer) != 0) {
        return -1;
    }

    return 0;
}