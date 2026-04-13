#ifndef BITMAP_H
#define BITMAP_H

#include <stddef.h>
#include <stdint.h>

int bitmap_get(const uint8_t *bitmap, int index);

void bitmap_set(uint8_t *bitmap, int index);

void bitmap_clear(uint8_t *bitmap, int index);

int bitmap_find_first_free(const uint8_t *bitmap, int total_bits);

int bitmap_allocate(uint8_t *bitmap, int total_bits);
 
int bitmap_free(uint8_t *bitmap, int total_bits, int index);

#endif