#include "paging.h"
#include "pmm.h"

uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));

static inline void load_page_directory(uint32_t phys_addr) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(phys_addr));
}

static inline void enable_paging() {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

void init_paging() {
    // zero page directory
    for(int i = 0; i < PAGE_ENTRIES; i++) page_directory[i] = 0;

    // identity map first 4 MB
    uint32_t table_frame = pmm_alloc_frame();
    uint32_t* first_table = (uint32_t*)table_frame;
    for(int i = 0; i < 1024; i++) first_table[i] = (i * PAGE_SIZE) | 3;
    page_directory[0] = table_frame | 3;

    load_page_directory((uint32_t)page_directory);
    enable_paging();
}
