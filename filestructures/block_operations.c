#include "block_operation.h"
#include "constants.h"

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
     * Since each item is 1 byte, we expect BLOCK_SIZE items.
     */

    //returns how many items were successfully read.
    size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, disk);
    if (bytes_read != BLOCK_SIZE) {
        return -1;
    }
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