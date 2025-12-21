#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

// set a block of memory to a specific value
void *flop_memset(void *dest, int value, size_t size);

// compare two memory blocks
int flop_memcmp(const void *ptr1, const void *ptr2, size_t num);

// copy a block of memory from src to dest
void *flop_memcpy(void *dest, const void *src, size_t n);

// move a block of memory from src to dest
void *flop_memmove(void *dest, const void *src, size_t n);

#endif // UTILS_H
