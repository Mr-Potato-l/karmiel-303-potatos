#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 0x1000
#define PAGE_ENTRIES 1024

extern uint32_t page_directory[PAGE_ENTRIES];

void init_paging();          // sets up page directory & identity map
static inline void load_page_directory(uint32_t phys_addr);
static inline void enable_paging();

#endif