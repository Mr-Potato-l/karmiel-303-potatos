#include "vmm.h"
#include "pmm.h"
#include "paging.h"

typedef struct page {
    uint32_t present:1;
    uint32_t rw:1;
    uint32_t user:1;
    uint32_t accessed:1;
    uint32_t dirty:1;
    uint32_t unused:7;
    uint32_t frame:20;
} page_t;

void *memset(void *dst, int val, size_t count);

page_t* get_page(uint32_t virt, int create) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    uint32_t pd_entry = page_directory[pd_idx];

    if(pd_entry & 0x1) {
        uint32_t pt_phys = pd_entry & 0xFFFFF000;
        page_t* pt = (page_t*) pt_phys;
        return &pt[pt_idx];
    } else if(create) {
        uint32_t phys = pmm_alloc_frame();
        if(!phys) return 0;
        memset((void*)phys, 0, PAGE_SIZE);
        page_directory[pd_idx] = phys | 0x7;
        page_t* pt = (page_t*)phys;
        return &pt[pt_idx];
    } else {
        return 0;
    }
}

void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    page_t* page = get_page(virt, 1);
    if(!page) return;
    page->present = flags & 0x1;
    page->rw = (flags & 0x2) ? 1 : 0;
    page->user = (flags & 0x4) ? 1 : 0;
    page->frame = phys >> 12;
}

void unmap_page(uint32_t virt) {
    page_t* page = get_page(virt, 0);
    if(!page) return;
    if(page->present) {
        pmm_free_frame(page->frame << 12);
        page->present = 0;
        page->frame = 0;
    }
}

uint32_t alloc_frame_for_page(page_t* page, int is_kernel, int is_writable) {
    if(page->frame) return page->frame << 12;
    uint32_t phys = pmm_alloc_frame();
    if(!phys) return 0;
    page->present = 1;
    page->rw = is_writable ? 1 : 0;
    page->user = is_kernel ? 0 : 1;
    page->frame = phys >> 12;
    return phys;
}

void free_frame_for_page(page_t* page) {
    if(!page->frame) return;
    pmm_free_frame(page->frame << 12);
    page->frame = 0;
    page->present = 0;
}

void vmm_init() {
    // could set up kernel heap mappings here if needed
}

void *memset(void *dst, int val, size_t count) {
    unsigned char *ptr = (unsigned char*)dst;
    while(count--) {
        *ptr++ = (unsigned char)val;
    }
    return dst;
}