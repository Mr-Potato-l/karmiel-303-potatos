#include "pmm.h"
#include <stdint.h>

// Page size: 4 KB (0x1000 bytes)
#define PAGE_SIZE 0x1000u

// Multiboot memory map entry structure
// Used by bootloader to describe available/reserved memory regions
typedef struct multiboot_mmap_entry {
    uint32_t size;      // Size of this entry
    uint64_t addr;      // Start address of memory region
    uint64_t len;       // Length of memory region
    uint32_t type;      // Type: 1 = available RAM, other = reserved
} __attribute__((packed)) multiboot_mmap_entry_t;

// Global PMM state
static uint32_t *frames_bitmap = 0;         // Bitmap tracking frame allocation (1=used, 0=free)
static uint32_t frames_bitmap_u32 = 0;     // Size of bitmap in 32-bit integers
static uint32_t total_frames = 0;           // Total number of frames being managed
static uint32_t managed_phys_bytes = 0;    // Total physical memory size in bytes

// Macros to convert between frame indices and bitmap positions
// Each frame is represented by 1 bit, so 32 frames per uint32
#define INDEX_FROM_BIT(a) ((a) / 32U)       // Which uint32 element in the bitmap
#define OFFSET_FROM_BIT(a) ((a) % 32U)      // Which bit within that uint32


// ============================================================================
// BITMAP OPERATIONS - Manage individual frame allocation status
// ============================================================================

// Mark a frame as used by setting its bit to 1
static inline void bitmap_set(uint32_t frame_idx) {
    frames_bitmap[INDEX_FROM_BIT(frame_idx)] |= (1U << OFFSET_FROM_BIT(frame_idx));
}

// Mark a frame as free by clearing its bit to 0
static inline void bitmap_clear(uint32_t frame_idx) {
    frames_bitmap[INDEX_FROM_BIT(frame_idx)] &= ~(1U << OFFSET_FROM_BIT(frame_idx));
}

// Check if a frame is used (returns 1 if used, 0 if free)
static inline int bitmap_test(uint32_t frame_idx) {
    return (frames_bitmap[INDEX_FROM_BIT(frame_idx)] & (1U << OFFSET_FROM_BIT(frame_idx))) != 0;
}

// ============================================================================
// FRAME SEARCH - Find available frames
// ============================================================================

// Find the first free frame by scanning the bitmap
// Returns frame index, or -1 if no free frames available
static uint32_t find_first_free_frame(void) {
    // Iterate through each uint32 element in bitmap
    for (uint32_t i = 0; i < frames_bitmap_u32; ++i) {
        // Skip if this element is completely full (all bits set)
        if (frames_bitmap[i] != 0xFFFFFFFFU) {
            // Check each bit individually for a free frame
            for (int bit = 0; bit < 32; ++bit) {
                uint32_t mask = 1U << bit;
                // Found a free bit (0)
                if (!(frames_bitmap[i] & mask)) {
                    uint32_t frame = i * 32U + bit;
                    // Ensure frame index is within valid range
                    if (frame < total_frames) return frame;
                    return (uint32_t)-1;
                }
            }
        }
    }
    return (uint32_t)-1;
}

// ============================================================================
// ADDRESS CONVERSION - Convert between frame indices and physical addresses
// ============================================================================

// Convert frame index to physical address (multiply by page size)
static inline uint32_t frame_to_phys(uint32_t frame_idx) {
    return frame_idx * PAGE_SIZE;
}

// Convert physical address to frame index (divide by page size)
static inline uint32_t phys_to_frame(uint32_t phys) {
    return phys / PAGE_SIZE;
}


// ============================================================================
// RANGE MARKING - Mark contiguous memory regions as used or free
// ============================================================================

// Mark a range of physical memory as used
// Aligns to page boundaries and sets all corresponding frame bits
void pmm_mark_used(uint32_t phys_addr, uint32_t size) {
    if (size == 0) return;
    // Align start address down to page boundary
    uint32_t start = phys_to_frame(phys_addr & ~(PAGE_SIZE - 1));
    // Align end address up to page boundary
    uint32_t end = phys_to_frame((phys_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    // Mark all frames in range as used
    for (uint32_t f = start; f < end && f < total_frames; ++f) {
        bitmap_set(f);
    }
}

// Mark a range of physical memory as free
// Aligns to page boundaries and clears all corresponding frame bits
void pmm_mark_free(uint32_t phys_addr, uint32_t size) {
    if (size == 0) return;
    // Align start address down to page boundary
    uint32_t start = phys_to_frame(phys_addr & ~(PAGE_SIZE - 1));
    // Align end address up to page boundary
    uint32_t end = phys_to_frame((phys_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    // Mark all frames in range as free
    for (uint32_t f = start; f < end && f < total_frames; ++f) {
        bitmap_clear(f);
    }
}


// ============================================================================
// ALLOCATION & DEALLOCATION - Allocate and free individual frames
// ============================================================================

// Allocate a single free physical frame
// Returns physical address of allocated frame, or 0 if no frames available
uint32_t pmm_alloc_frame(void) {
    // Find first free frame in bitmap
    uint32_t fi = find_first_free_frame();
    if (fi == (uint32_t)-1) return 0;  // No free frames
    // Mark the frame as used
    bitmap_set(fi);
    // Return the physical address of this frame
    return frame_to_phys(fi);
}

// Deallocate a physical frame (free it back to the pool)
void pmm_free_frame(uint32_t phys_addr) {
    // Convert address to frame index
    uint32_t fi = phys_to_frame(phys_addr);
    // Safety check: ensure frame index is valid
    if (fi >= total_frames) return;
    // Mark the frame as free
    bitmap_clear(fi);
}


// ============================================================================
// INITIALIZATION - Set up the physical memory manager
// ============================================================================

// Initialize PMM with multiboot memory map
// Tracks which memory regions are available and which are reserved
void pmm_init(uint32_t mmap_addr, uint32_t mmap_length, uint32_t max_phys) {
    uint32_t highest = 0;
    uint32_t pos = 0;

    // STEP 1: Scan memory map to find the highest memory address
    while (pos < mmap_length) {
        multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)(uintptr_t)(mmap_addr + pos);
        // Track the highest memory address we've seen
        if ((uint32_t)(e->addr + e->len) > highest) {
            uint64_t end = e->addr + e->len;
            // Cap to 32-bit address space
            if (end > 0xFFFFFFFFULL) end = 0xFFFFFFFFULL;
            if ((uint32_t)end > highest) highest = (uint32_t)end;
        }
        // Move to next entry (accounting for entry size field)
        pos += e->size + sizeof(e->size);
    }

    // STEP 2: Mark kernel structures as reserved
    // Get address of page directory from linker script
    extern uint32_t page_directory;
    uint32_t pd_phys = (uint32_t)&page_directory;

    // Mark page directory and page tables as used (2 pages)
    pmm_mark_used(pd_phys, PAGE_SIZE * 2);
    // Mark kernel binary as used (rough estimate: 1MB–4MB region)
    pmm_mark_used(0x100000, 0x400000);

    // STEP 3: Calculate total frames to manage
    if (max_phys && max_phys < highest) highest = max_phys;
    managed_phys_bytes = highest;
    // Round up to ensure we cover all memory
    total_frames = (managed_phys_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // STEP 4: Initialize bitmap storage and zero it
    frames_bitmap = frames_bitmap_static;
    frames_bitmap_u32 = sizeof(frames_bitmap_static) / 4;
    // Clear bitmap
    for (uint32_t i = 0; i < frames_bitmap_u32; ++i) frames_bitmap[i] = 0;

    // STEP 5: Mark ALL frames as used initially
    for (uint32_t f = 0; f < total_frames; ++f) bitmap_set(f);

    // STEP 6: Parse memory map again and mark free regions
    pos = 0;
    while (pos < mmap_length) {
        multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)(uintptr_t)(mmap_addr + pos);
        // Type 1 = available RAM
        if (e->type == 1) {
            uint32_t start = (uint32_t)(e->addr & 0xFFFFFFFFULL);
            uint32_t len = (uint32_t)(e->len & 0xFFFFFFFFULL);
            // Align to page boundaries
            uint32_t s_frame = phys_to_frame(start & ~(PAGE_SIZE - 1));
            uint32_t e_frame = phys_to_frame((start + len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
            // Mark all frames in this region as free
            for (uint32_t f = s_frame; f < e_frame && f < total_frames; ++f) {
                bitmap_clear(f);
            }
        }
        // Move to next entry
        pos += e->size + sizeof(e->size);
    }
}


// ============================================================================
// STATISTICS - Query memory allocation status
// ============================================================================

// Get total number of frames being managed
uint32_t pmm_total_frames(void) {
    return total_frames;
}

// Count and return number of free frames
uint32_t pmm_total_free_frames(void) {
    uint32_t free = 0;
    // Count frames that are not allocated
    for (uint32_t f = 0; f < total_frames; ++f) {
        if (!bitmap_test(f)) ++free;
    }
    return free;
}

// Print memory statistics to the terminal
void pmm_dump_stats(void) {
    extern void terminal_writestring(const char*);
    extern char* inttoa(int, char*);
    char buf[64];

    // Print total frames
    terminal_writestring("PMM: total frames: ");
    inttoa((int)total_frames, buf);
    terminal_writestring(buf);

    // Print free frames
    terminal_writestring("\nPMM: free frames: ");
    inttoa((int)pmm_total_free_frames(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
}
