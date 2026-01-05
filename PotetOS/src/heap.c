#include "heap.h"
#include "paging.h"
#include "pmm.h"
#include "vmm.h"

#define PAGE_SIZE 0x1000

static uint32_t heap_current = KERNEL_HEAP_START;
static uint32_t heap_end     = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;

void heap_init(void) {
    heap_current = KERNEL_HEAP_START;
}

static void heap_map_page(uint32_t virt) {
    // Try to get the page entry without creating it; if it doesn't exist,
    // allocate a physical frame and map the page.
    page_t* p = get_page(virt, 0);
    if (p == 0) {
        uint32_t phys = pmm_alloc_frame();
        map_page(virt, phys, 0x3); // present | rw
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return 0;

    // Align to 8 bytes
    size = (size + 7) & ~7;

    uint32_t addr = heap_current;
    uint32_t end  = heap_current + size;

    while (addr < end) {
        heap_map_page(addr);
        addr += PAGE_SIZE;
    }

    void* ret = (void*)heap_current;
    heap_current = end;

    return ret;
}
