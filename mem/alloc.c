#include "pmm.h"
#include "alloc.h"
#include "utils.h"
#include "../lib/logging.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static box_t* boxes = NULL;
static int heap_initialized = 0;

static box_t* heap_create_box(void) {
    void* page = pmm_alloc_page();

    if (!page) {
        return NULL;
    }

    box_t* box = (box_t*) page;
    box->next = boxes;
    box->page = page;

    uintptr_t base = (uintptr_t) page;

    size_t metadata_size = sizeof(box_t);
    size_t available = PAGE_SIZE - metadata_size;
    int max_blocks = available / (BLOCK_SIZE + sizeof(block_t));

    if (max_blocks <= 0) {
        log("alloc test: cannot fit blocks in page\n", RED);
        return NULL;
    }

    box->blocks = (block_t*) (base + sizeof(box_t));
    box->data_pointer = (void*) (base + sizeof(box_t) + max_blocks * sizeof(block_t));

    for (int i = 0; i < max_blocks; i++) {
        box->blocks[i].used = false;
    }

    boxes = box;
    return box;
}

static void* box_alloc(box_t* box, size_t size) {
    size_t total = size + OBJECT_ALIGN;
    int needed = (total + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int run = 0;
    int start = -1;

    for (int i = 0; i < BLOCKS_PER_BOX; i++) {
        if (!box->blocks[i].used) {
            if (run == 0) {
                start = i;
            }
            run++;
            if (run >= needed) {
                break;
            }
        } else {
            run = 0;
            start = -1;
        }
    }

    if (run < needed) {
        return NULL;
    }

    for (int i = start; i < start + needed; i++) {
        box->blocks[i].used = true;
    }

    uintptr_t mem = (uintptr_t) box->data_pointer + (start * BLOCK_SIZE);

    object_t* obj = (object_t*) mem;
    obj->box = box;
    obj->size = size;

    return (void*) (mem + OBJECT_ALIGN);
}

void heap_init(void) {
    if (heap_initialized) {
        log("heap_init called twice\n", RED);
        return;
    }

    boxes = NULL;

    if (!heap_create_box()) {
        log("heap_init failed\n", RED);
        return;
    }

    heap_initialized = 1;
    log("heap: init - ok\n", GREEN);
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (size > PAGE_SIZE) {
        size_t pages = (size + OBJECT_ALIGN + PAGE_SIZE - 1) / PAGE_SIZE;

        void* mem = pmm_alloc_pages(0, pages);
        if (!mem) {
            return NULL;
        }

        object_t* obj = (object_t*) mem;
        obj->box = NULL;
        obj->size = size;

        return (void*) ((uintptr_t) mem + OBJECT_ALIGN);
    }

    if (!heap_initialized) {
        heap_init();
        if (!boxes) {
            return NULL;
        }
    }

    // look through boxes until we find a box with enough blocks
    // TODO: there is likely a better way to do this
    for (box_t* b = boxes; b; b = b->next) {
        void* r = box_alloc(b, size);
        if (r) {
            return r;
        }
    }

    box_t* b = heap_create_box();
    if (!b) {
        return NULL;
    }

    return box_alloc(b, size);
}

static int block_index(box_t* box, void* mem_ptr) {
    uintptr_t base = (uintptr_t) box->data_pointer;
    uintptr_t p = (uintptr_t) mem_ptr;

    if (p < base) {
        return -1;
    }

    uintptr_t diff = p - base;
    if (diff % BLOCK_SIZE) {
        return -1;
    }

    int idx = diff / BLOCK_SIZE;
    if (idx < 0 || idx >= BLOCKS_PER_BOX) {
        return -1;
    }

    return idx;
}

void kfree(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }

    object_t* obj = (object_t*) ((uintptr_t) ptr - OBJECT_ALIGN);

    if (!obj->box) {
        size_t pages = (obj->size + OBJECT_ALIGN + PAGE_SIZE - 1) / PAGE_SIZE;
        pmm_free_pages((void*) obj, 0, pages);
        return;
    }

    box_t* b = obj->box;

    size_t total = obj->size + OBJECT_ALIGN;
    int needed = (total + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int idx = block_index(b, (void*) obj);

    if (idx < 0) {
        return;
    }

    for (int i = idx; i < idx + needed && i < BLOCKS_PER_BOX; i++) {
        b->blocks[i].used = false;
    }
}

void* kcalloc(size_t n, size_t s) {
    size_t t = n * s;
    void* p = kmalloc(t);
    if (!p) {
        return NULL;
    }

    flop_memset(p, 0, t);
    return p;
}

void* krealloc(void* ptr, size_t new_size, size_t old_size) {
    if (!ptr) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr, old_size);
        return NULL;
    }

    object_t* old = (object_t*) ((uintptr_t) ptr - OBJECT_ALIGN);

    void* n = kmalloc(new_size);
    if (!n) {
        return NULL;
    }

    size_t copy = (old->size < new_size) ? old->size : new_size;

    flop_memcpy(n, ptr, copy);
    kfree(ptr, old_size);
    return n;
}

int kmalloc_memtest(void) {
    log("alloc test: begin\n", GREEN);

    void* a = kmalloc(64);
    if (!a) {
        return -1;
    }
    kfree(a, 64);
    log("alloc test: alloc/free ok\n", GREEN);

    void* b = kmalloc(64);
    if (!b) {
        return -1;
    }
    kfree(b, 64);
    log("alloc test: reuse ok\n", GREEN);

    uint32_t* c = kcalloc(32, sizeof(uint32_t));
    if (!c) {
        return -1;
    }
    kfree(c, 32 * sizeof(uint32_t));
    log("alloc test: kcalloc ok\n", GREEN);

    uint8_t* d = kmalloc(32);
    if (!d) {
        return -1;
    }
    for (int i = 0; i < 32; i++)
        d[i] = i;
    uint8_t* d2 = krealloc(d, 128, 32);
    if (!d2) {
        return -1;
    }
    kfree(d2, 128);
    log("alloc test: krealloc grow ok\n", GREEN);

    size_t big_size = PAGE_SIZE * 3 + 100;
    uint8_t* big = kmalloc(big_size);
    if (!big) {
        return -1;
    }
    kfree(big, big_size);
    log("alloc test: large alloc ok\n", GREEN);

    log("alloc test: completed.\n", GREEN);
    return 0;
}
