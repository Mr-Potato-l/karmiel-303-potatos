#include "malloc.h"

void malloc_init(multiboot_memory_map_t* mmt,  uint32_t magic) {
    // Initialize the malloc system (if needed)
    malloc_header first = (block_header*)heap_start;
    first->size = mmt - heap_start - sizeof(block_header);
    first->free = true;
    first->next = NULL;
}