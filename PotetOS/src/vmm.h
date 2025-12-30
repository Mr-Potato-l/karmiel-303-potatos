#ifndef VMM_H
#define VMM_H

#include <stdint.h>

typedef struct page page_t;

void vmm_init();
page_t* get_page(uint32_t virt, int create);
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void unmap_page(uint32_t virt);
uint32_t alloc_frame_for_page(page_t* page, int is_kernel, int is_writable);
void free_frame_for_page(page_t* page);

#endif