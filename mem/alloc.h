#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define BLOCK_SIZE 32
#define BLOCKS_PER_BOX (PAGE_SIZE / BLOCK_SIZE)

typedef struct box box_t;

typedef struct object {
    box_t* box;
    size_t size;
} object_t;

#define OBJECT_ALIGN ((sizeof(object_t) + (BLOCK_SIZE - 1)) & ~(BLOCK_SIZE - 1))

typedef struct block {
    bool used;
} block_t;

struct box {
    struct box* next;
    void* page;
    block_t* blocks;
    void* data_pointer;
};

void* kmalloc(size_t size);
void kfree(void* ptr, size_t size);
void* kcalloc(size_t n, size_t s);
void* krealloc(void* ptr, size_t new_size, size_t old_size);
int kmalloc_memtest(void);
void heap_init(void);
#endif
