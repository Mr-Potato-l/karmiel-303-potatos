#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include <stddef.h>

// ============================================================================
// PAGE TABLE ENTRY STRUCTURE
// ============================================================================

// Page table entry (PTE) - 32-bit format
// Contains physical frame address and permission flags
typedef struct page {
    uint32_t present:1;    // Bit 0: Is page present in memory?
    uint32_t rw:1;         // Bit 1: Is page writable? (0=read-only, 1=read/write)
    uint32_t user:1;       // Bit 2: Is page accessible by user mode? (0=kernel, 1=user)
    uint32_t accessed:1;   // Bit 3: Has CPU accessed this page?
    uint32_t dirty:1;      // Bit 4: Has CPU written to this page?
    uint32_t unused:7;     // Bits 5-11: Unused/reserved
    uint32_t frame:20;     // Bits 12-31: Physical frame address (>> 12)
} page_t;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// memset implementation
void *memset(void *dst, int val, size_t count);

// ============================================================================
// PAGE TABLE MANAGEMENT
// ============================================================================

// Get or create page table entry for a virtual address
// virt: virtual address
// create: if 1, allocate page table if it doesn't exist; if 0, just lookup
// Returns pointer to page entry, or NULL if not found/couldn't create
page_t* get_page(uint32_t virt, int create) {
    // Extract page directory index (upper 10 bits)
    uint32_t pd_idx = virt >> 22;
    
    // Extract page table index (middle 10 bits)
    uint32_t pt_idx = (virt >> 12) & 0x3FF;
    
    // Get page directory entry
    uint32_t pd_entry = page_directory[pd_idx];

    // Check if page table is present
    if (pd_entry & 0x1) {
        // Page table exists, extract its physical address
        uint32_t pt_phys = pd_entry & 0xFFFFF000;
        page_t* pt = (page_t*)pt_phys;
        // Return pointer to the page table entry
        return &pt[pt_idx];
    } else if (create) {
        // Page table doesn't exist, allocate one if requested
        uint32_t phys = pmm_alloc_frame();
        if (!phys) return 0;  // Allocation failed
        
        // Zero out the new page table
        memset((void*)phys, 0, PAGE_SIZE);
        
        // Link page table into page directory
        // Flags: present (1) | writable (2) | user (4) = 7
        page_directory[pd_idx] = phys | 0x7;
        
        // Return pointer to page table entry
        page_t* pt = (page_t*)phys;
        return &pt[pt_idx];
    } else {
        // Page table doesn't exist and we're not creating one
        return 0;
    }
}

// ============================================================================
// PAGE MAPPING
// ============================================================================

// Map a virtual address to a physical address with given flags
void map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // Get (or create) the page table entry
    page_t* page = get_page(virt, 1);
    if (!page) return;  // Failed to get page entry
    
    // Set present bit
    page->present = flags & 0x1;
    
    // Set read/write bit
    page->rw = (flags & 0x2) ? 1 : 0;
    
    // Set user/kernel bit
    page->user = (flags & 0x4) ? 1 : 0;
    
    // Set physical frame address (bits 12-31, so divide by 4096)
    page->frame = phys >> 12;
}

// Unmap a virtual address and free its physical frame
void unmap_page(uint32_t virt) {
    // Get page entry without creating
    page_t* page = get_page(virt, 0);
    if (!page) return;  // Page doesn't exist
    
    // If page is present, free its physical frame
    if (page->present) {
        pmm_free_frame(page->frame << 12);
        page->present = 0;
        page->frame = 0;
    }
}

// ============================================================================
// FRAME ALLOCATION FOR PAGES
// ============================================================================

// Allocate a physical frame for a page and set permissions
// page: pointer to page table entry
// is_kernel: 1 for kernel page, 0 for user page
// is_writable: 1 if page should be writable, 0 if read-only
// Returns physical address of allocated frame, or 0 on failure
uint32_t alloc_frame_for_page(page_t* page, int is_kernel, int is_writable) {
    // If frame already allocated, return its address
    if (page->frame) return page->frame << 12;
    
    // Allocate new physical frame
    uint32_t phys = pmm_alloc_frame();
    if (!phys) return 0;  // Allocation failed
    
    // Mark page as present
    page->present = 1;
    
    // Set read/write permission
    page->rw = is_writable ? 1 : 0;
    
    // Set kernel/user mode
    page->user = is_kernel ? 0 : 1;
    
    // Set physical frame address
    page->frame = phys >> 12;
    
    return phys;
}

// Deallocate physical frame from a page
void free_frame_for_page(page_t* page) {
    // Check if frame is allocated
    if (!page->frame) return;
    
    // Free the physical frame
    pmm_free_frame(page->frame << 12);
    
    // Clear frame address and present bit
    page->frame = 0;
    page->present = 0;
}

// ============================================================================
// INITIALIZATION & UTILITIES
// ============================================================================

// Initialize the virtual memory manager
void vmm_init(void) {
    // Placeholder for kernel heap setup or other VMM initialization
}

// Fill memory with a byte value
// Common utility for zeroing memory
void *memset(void *dst, int val, size_t count) {
    unsigned char *ptr = (unsigned char*)dst;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dst;
}