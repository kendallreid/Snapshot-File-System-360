#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"

int main(void)
{
    //16 bits
    uint8_t map[2] = {0}; 

    int first = bitmap_allocate(map, 16);
    int second = bitmap_allocate(map, 16);

    printf("first allocated bit: %d\n", first);
    printf("second allocated bit: %d\n", second);

    printf("bit 0 = %d\n", bitmap_get(map, 0));
    printf("bit 1 = %d\n", bitmap_get(map, 1));

    bitmap_free(map, 16, first);

    printf("after freeing first bit:\n");
    printf("bit 0 = %d\n", bitmap_get(map, 0));

    int again = bitmap_allocate(map, 16);
    printf("allocated again: %d\n", again);

    return 0;
}