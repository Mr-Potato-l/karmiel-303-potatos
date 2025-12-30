#ifndef MALLOC_H
#define MALLOC_H

#include "paging.h"
#include <stdint.h>

struct malloc_header {
    size_t size;
    struct malloc_header* next;
    struct malloc_header* prev;
    bool free;
}typedef malloc_header malloc_header;

void malloc_init();
void* malloc(size_t size);
void free(void* ptr);

#endif