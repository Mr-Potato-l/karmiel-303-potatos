#include "paging.h"
#include "pmm.h"

// ============================================================================
// PAGING STRUCTURES
// ============================================================================

// Page directory: maps 1024 virtual 4MB regions to page tables
// Aligned to 4KB boundary as required by x86 architecture
// Each entry points to a page table (or marks region as not present)
uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));

// ============================================================================
// PAGE DIRECTORY & TABLE MANAGEMENT
// ============================================================================

// Load page directory address into CR3 (page directory base register)
// The CPU uses CR3 to locate the page directory during address translation
static inline void load_page_directory(uint32_t phys_addr) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(phys_addr));
}

// ============================================================================
// PAGING ENABLE
// ============================================================================

// Enable paging by setting the PG bit (bit 31) in CR0
// After this, all memory accesses use virtual-to-physical translation
static inline void enable_paging() {
    uint32_t cr0;
    // Read current CR0 value
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    
    // Set bit 31 (paging enable bit)
    // We use bitwise OR since inline asm can't directly set this bit
    cr0 |= 0x80000000;
    
    // Write modified CR0 back to enable paging
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

// ============================================================================
// INITIALIZATION
// ============================================================================

// Initialize and enable paging
// Sets up identity mapping for first 4MB of memory
void init_paging() {
    // STEP 1: Clear all entries in page directory
    for(int i = 0; i < PAGE_ENTRIES; i++) {
        page_directory[i] = 0;
    }

    // STEP 2: Allocate a physical frame for the first page table
    // This will map the first 4MB of virtual memory
    uint32_t table_frame = pmm_alloc_frame();
    uint32_t* first_table = (uint32_t*)table_frame;

    // STEP 3: Create identity mapping entries in the first page table
    // Each entry maps a virtual address to the same physical address
    // Flags: present (1) | writable (2) = 3
    for(int i = 0; i < 1024; i++) {
        first_table[i] = (i * PAGE_SIZE) | 3;
    }

    // STEP 4: Link page table into page directory
    // Entry 0 of page directory points to first page table
    // Flags: present (1) | writable (2) | user (1) = 3
    page_directory[0] = table_frame | 3;

    // STEP 5: Load page directory into CR3
    load_page_directory((uint32_t)page_directory);

    // STEP 6: Enable paging
    enable_paging();
}
