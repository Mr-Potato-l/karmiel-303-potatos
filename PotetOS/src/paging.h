#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000
#define PAGE_ENTRIES 1024

typedef struct page {
    uint32_t present    : 1;  // Page present in memory
    uint32_t rw         : 1;  // Read-only if clear, readwrite if set
    uint32_t user       : 1;  // Supervisor level only if clear
    uint32_t accessed   : 1;  // Has the page been accessed since last refresh?
    uint32_t dirty      : 1;  // Has the page been written to since last refresh?
    uint32_t unused     : 7;
    uint32_t frame      : 20; // Frame address (shifted right 12 bits)
} page_t;

void init_paging(uint32_t phys_mem_bytes); // call once with an estimate or real value
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void unmap_page(uint32_t virt);
page_t* get_page(uint32_t virt, int create);

uint32_t alloc_frame_for_page(page_t *page, int is_kernel, int is_writable);
void free_frame_for_page(page_t *page);

#endif
