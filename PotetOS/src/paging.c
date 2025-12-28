#include "paging.h"
#include "io.h"
#include "pmm.h"

#define ALIGN_4K __attribute__((aligned(4096)))

uint32_t ALIGN_4K page_directory[PAGE_ENTRIES];


void *memset(void *dst, int val, size_t count) {
    unsigned char *ptr = dst;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dst;
}

page_t* get_page(uint32_t virt, int create) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;

    uint32_t pd_entry = page_directory[pd_idx];
    if (pd_entry & 0x1) {
        uint32_t pt_phys = pd_entry & 0xFFFFF000;
        page_t *pt = (page_t*) pt_phys;
        return &pt[pt_idx];
    } else if (create) {
        uint32_t phys = pmm_alloc_frame();
        if (!phys) return 0;
        memset((void*)phys, 0, PAGE_SIZE);
        page_directory[pd_idx] = phys | 0x7;
        page_t *pt = (page_t*)phys;
        return &pt[pt_idx];
    } else {
        return 0;
    }
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    page_t *page = get_page(virt, 1);
    if (!page) return;
    page->present = (flags & 0x1);
    page->rw = (flags & 0x2) ? 1 : 0;
    page->user = (flags & 0x4) ? 1 : 0;
    page->frame = phys >> 12;
}

void unmap_page(uint32_t virt) {
    page_t *p = get_page(virt, 0);
    if (!p) return;
    if (p->present) {
        uint32_t frame_addr = p->frame << 12;
        pmm_free_frame(frame_addr);
        p->present = 0;
        p->frame = 0;
    }
}

uint32_t alloc_frame_for_page(page_t *page, int is_kernel, int is_writable) {
    if (page->frame) return page->frame << 12;
    uint32_t phys = pmm_alloc_frame();
    if (!phys) return 0;
    page->present = 1;
    page->rw = is_writable ? 1 : 0;
    page->user = is_kernel ? 0 : 1;
    page->frame = phys >> 12;
    return phys;
}

void free_frame_for_page(page_t *page) {
    if (!page->frame) return;
    uint32_t addr = page->frame << 12;
    pmm_free_frame(addr);
    page->frame = 0;
    page->present = 0;
}

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
    for (int i = 0; i < PAGE_ENTRIES; i++) page_directory[i] = 0;

    uint32_t first_table_phys = (uint32_t)((uint8_t*)page_directory + PAGE_SIZE);
    uint32_t table_frame = pmm_alloc_frame();
    if (!table_frame) {
        return;
    }
    uint32_t* first_table = (uint32_t*)table_frame;
    for (int i = 0; i < 1024; i++) {
        first_table[i] = (i * PAGE_SIZE) | 3;
    }
    page_directory[0] = table_frame | 3;

    load_page_directory((uint32_t)page_directory);
    enable_paging();
}