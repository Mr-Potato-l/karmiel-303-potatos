#include <stdint.h>

uint32_t __attribute__((aligned(4096))) page_directory[1024];
uint32_t __attribute__((aligned(4096))) first_page_table[1024];

void paging_init()
{
    // Identity map the first 4MB (each entry maps 4KB)
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3;
        // 3 = present + writable
    }

    // Clear page directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
        // writable but not present
    }

    // Insert our page table into PDE 0
    page_directory[0] = ((uint32_t)first_page_table) | 3;
}

extern uint32_t page_directory[];

void paging_enable()
{
    uint32_t pd_addr = (uint32_t)page_directory;

    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_addr));

    __asm__ volatile(
    "mov %cr0, %eax\n"
    "or $0x80000000, %eax\n"
    "mov %eax, %cr0\n"
    );
}

void init_paging()
{
    paging_init();
    paging_enable();
}