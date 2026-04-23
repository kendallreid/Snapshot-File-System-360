#include "bitmap.h"

 // returns the value of the bit at the given index.
 // returns 1 if the bit is set, 0 if it is not set.
int bitmap_get(const uint8_t *bitmap, int index)
{
    
    if (bitmap == NULL || index < 0) {
        return 0;
    }

    // figure out which byte contains this bit.
    int byte_index = index / 8;

    // figure out which bit inside that byte we want. 
    int bit_index = index % 8;

    // shift the target bit down to the rightmost position
    // then mask with 1 so we only keep that single bit.
    return (bitmap[byte_index] >> bit_index) & 1;
}

// sets the bit at the given index to 1.
void bitmap_set(uint8_t *bitmap, int index)
{
    if (bitmap == NULL || index < 0) {
        return;
    }

    int byte_index = index / 8;

    int bit_index = index % 8;

    // create a mask with a 1 at bit index
    // then turn that bit on.
    bitmap[byte_index] |= (uint8_t)(1U << bit_index);
}

// clears the bit at the given index to 0.
void bitmap_clear(uint8_t *bitmap, int index)
{
    
    if (bitmap == NULL || index < 0) {
        return;
    }

    
    int byte_index = index / 8;

    int bit_index = index % 8;

    // create a mask with a 1 at bit_index,
    // invert it so that bit becomes 0 and all others are 1
    // then turn that bit off.
    bitmap[byte_index] &= (uint8_t)~(1U << bit_index);
}

// scans the bitmap from left to right by index number
// and returns the first bit whose value is 0.
int bitmap_find_first_free(const uint8_t *bitmap, int total_bits)
{
    if (bitmap == NULL || total_bits <= 0) {
        return -1;
    }

    // check each bit one by one 
    for (int i = 0; i < total_bits; i++) {
        /* If this bit is 0, it is free. */
        if (bitmap_get(bitmap, i) == 0) {
            return i;
        }
    }

    // no free bit found
    return -1;
}

// Finds the first free bit marks it as used
// and returns its index.
int bitmap_allocate(uint8_t *bitmap, int total_bits)
{
    if (bitmap == NULL || total_bits <= 0) {
        return -1;
    }

    // find the first free bit in the bitmap
    int free_index = bitmap_find_first_free(bitmap, total_bits);

    // if there are no free bits, allocation fails.
    if (free_index == -1) {
        return -1;
    }

    // mark that bit as used 
    bitmap_set(bitmap, free_index);

    // return the allocated index 
    return free_index;
}

// Frees the bit at the given index by setting it back to 0.
int bitmap_free(uint8_t *bitmap, int total_bits, int index)
{
    if (bitmap == NULL || total_bits <= 0 || index < 0 || index >= total_bits) {
        return -1;
    }

    // clear the bit so it becomes free again 
    bitmap_clear(bitmap, index);

    return 0;
}