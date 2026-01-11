#include "Malloc.h"
#include "vmm.h"
#include "paging.h"
#include "pmm.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// MALLOC SYSTEM - Heap allocation with linked-list management
// ============================================================================

// Heap region boundaries
#define HEAP_START  0xE0000000      // Virtual address where heap starts
#define HEAP_MAX    0xF0000000      // Maximum heap size (256 MB)
#define HEAP_SIZE   (HEAP_MAX - HEAP_START)

// Global heap state
static malloc_header* heap_head = NULL;  // First block in the heap
static uint32_t heap_top = HEAP_START;   // Current heap boundary (for expansion)

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Align size to 4-byte boundary (most restrictive requirement)
static inline size_t align_size(size_t size) {
    return (size + 3) & ~3;
}

// Round up to nearest page (PAGE_SIZE = 0x1000)
static inline uint32_t round_up_page(uint32_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// ============================================================================
// MEMORY OPERATIONS
// ============================================================================

// Expand heap by mapping new virtual pages
// Returns 1 on success, 0 on failure
static int expand_heap(size_t size) {
    // Calculate pages needed
    uint32_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Check if expansion would exceed heap limit
    if (heap_top + (pages_needed * PAGE_SIZE) > HEAP_MAX) {
        return 0;  // Heap overflow
    }

    // Map each new page with a physical frame
    for (uint32_t i = 0; i < pages_needed; i++) {
        uint32_t virt_addr = heap_top + (i * PAGE_SIZE);
        uint32_t phys_addr = pmm_alloc_frame();
        
        if (!phys_addr) {
            return 0;  // Out of physical memory
        }

        // Map the virtual page to the physical frame
        // Flags: present (1) | writable (2) | user (4) = 7
        map_page(virt_addr, phys_addr, 7);
    }

    // Update heap top pointer
    heap_top += pages_needed * PAGE_SIZE;
    return 1;
}

// Find a free block that can fit 'size' bytes using first-fit strategy
// Returns pointer to the block, or NULL if not found
static malloc_header* find_free_block(size_t size) {
    malloc_header* current = heap_head;

    while (current) {
        // Check if this block is free and has enough space
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }

    return NULL;  // No suitable block found
}

// Split a block if it's larger than needed
// This reduces memory fragmentation
static void split_block(malloc_header* block, size_t size) {
    // Only split if remaining space can fit a header + some data
    if (block->size < size + sizeof(malloc_header) + 16) {
        return;  // Not worth splitting
    }

    // Create new block from the remainder
    malloc_header* new_block = (malloc_header*)((uint8_t*)block + sizeof(malloc_header) + size);
    new_block->size = block->size - size - sizeof(malloc_header);
    new_block->free = 1;
    new_block->next = block->next;
    new_block->prev = block;

    // Link new block into list
    if (block->next) {
        block->next->prev = new_block;
    }
    block->next = new_block;

    // Adjust original block size
    block->size = size;
}

// Merge adjacent free blocks to reduce fragmentation
static void coalesce_blocks() {
    if (!heap_head) return;

    malloc_header* current = heap_head;

    while (current && current->next) {
        // If current block and next block are both free, merge them
        if (current->free && current->next->free) {
            current->size += sizeof(malloc_header) + current->next->size;
            current->next = current->next->next;
            if (current->next) {
                current->next->prev = current;
            }
            // Don't advance, check if we can merge with the new next block too
        } else {
            current = current->next;
        }
    }
}

// ============================================================================
// MALLOC/FREE INTERFACE
// ============================================================================

// Initialize the malloc system
// Must be called once during kernel startup
void malloc_init(void) {
    // Expand heap to initial size (1 MB)
    if (!expand_heap(1024 * 1024)) {
        return;  // Failed to initialize heap
    }

    // Create the first free block
    heap_head = (malloc_header*)HEAP_START;
    heap_head->size = heap_top - HEAP_START - sizeof(malloc_header);
    heap_head->free = 1;
    heap_head->next = NULL;
    heap_head->prev = NULL;
}

// Allocate 'size' bytes of memory
// Returns pointer to allocated memory, or NULL on failure
void* malloc(size_t size) {
    if (size == 0) return NULL;

    // Align size for consistent allocation
    size = align_size(size);

    // First-fit allocation: find a suitable free block
    malloc_header* block = find_free_block(size);

    // If no suitable block found, expand the heap
    if (!block) {
        if (!expand_heap(size + sizeof(malloc_header))) {
            return NULL;  // Failed to expand heap
        }

        // After expansion, the last block should be a new free block
        // Find the last block and use it
        if (!heap_head) {
            malloc_init();  // Initialize if needed
        }

        // Find the end of the list
        block = heap_head;
        while (block->next) {
            block = block->next;
        }

        // If the last block is free, use it; otherwise create new block
        if (!block->free) {
            malloc_header* new_block = (malloc_header*)((uint8_t*)block + sizeof(malloc_header) + block->size);
            new_block->size = heap_top - (uint32_t)new_block - sizeof(malloc_header);
            new_block->free = 1;
            new_block->next = NULL;
            new_block->prev = block;
            block->next = new_block;
            block = new_block;
        }
    }

    // Split the block if it's larger than needed (reduce fragmentation)
    split_block(block, size);

    // Mark block as used and return pointer to data (after header)
    block->free = 0;
    return (void*)((uint8_t*)block + sizeof(malloc_header));
}

// Deallocate previously allocated memory
void free(void* ptr) {
    if (!ptr) return;  // NULL pointer is safe to free

    // Get the block header (located just before the data)
    malloc_header* block = (malloc_header*)((uint8_t*)ptr - sizeof(malloc_header));

    // Mark as free
    block->free = 1;

    // Coalesce adjacent free blocks
    coalesce_blocks();
}

// ============================================================================
// DEBUG & TESTING FUNCTIONS
// ============================================================================

// Forward declarations for print functions
extern void print(const char* fmt, ...);
extern void print_hex(uint32_t val);

// Dump malloc statistics for debugging
// Shows block list, fragmentation, and memory usage
void malloc_dump_stats(void) {
    if (!heap_head) {
        print("Malloc not initialized!\n");
        return;
    }

    print("\n=== MALLOC STATISTICS ===\n");
    print("Heap range: 0x");
    print_hex(HEAP_START);
    print(" - 0x");
    print_hex(HEAP_MAX);
    print("\nCurrent heap top: 0x");
    print_hex(heap_top);
    print("\nHeap used: ");
    print("{d}", (heap_top - HEAP_START) / 1024);
    print(" KB\n\n");

    // Walk through all blocks and print info
    malloc_header* current = heap_head;
    int block_num = 0;
    size_t total_used = 0;
    size_t total_free = 0;

    print("Block list:\n");
    while (current) {
        char status = current->free ? 'F' : 'A';  // F=Free, A=Allocated
        print("[{d}] {c} addr=0x", block_num, status);
        print_hex((uint32_t)current);
        print(" size={d} bytes", current->size);
        
        if (current->free) {
            total_free += current->size;
        } else {
            total_used += current->size;
        }
        
        print("\n");
        current = current->next;
        block_num++;
    }

    print("\nTotal allocated: {d} bytes\n", total_used);
    print("Total free: {d} bytes\n", total_free);
    print("Fragmentation: {d} blocks\n\n", block_num);
}

// Comprehensive malloc testing suite
// Tests allocation, deallocation, reallocation, and edge cases
// Returns 1 if all tests pass, 0 if any test fails
int malloc_test(void) {
    print("\n=== RUNNING MALLOC TESTS ===\n\n");

    // TEST 1: Basic allocation
    print("TEST 1: Basic allocation... ");
    int* p1 = (int*)malloc(sizeof(int));
    if (!p1) {
        print("FAILED (allocation returned NULL)\n");
        return 0;
    }
    *p1 = 42;
    if (*p1 != 42) {
        print("FAILED (data corruption)\n");
        return 0;
    }
    print("OK\n");

    // TEST 2: Multiple allocations
    print("TEST 2: Multiple allocations... ");
    int* p2 = (int*)malloc(sizeof(int));
    int* p3 = (int*)malloc(sizeof(int));
    int* p4 = (int*)malloc(sizeof(int));
    if (!p2 || !p3 || !p4) {
        print("FAILED (allocation returned NULL)\n");
        return 0;
    }
    *p2 = 100;
    *p3 = 200;
    *p4 = 300;
    if (*p1 != 42 || *p2 != 100 || *p3 != 200 || *p4 != 300) {
        print("FAILED (data corruption)\n");
        return 0;
    }
    print("OK\n");

    // TEST 3: Free and reallocation
    print("TEST 3: Free and reallocation... ");
    free(p2);
    int* p5 = (int*)malloc(sizeof(int));
    if (!p5) {
        print("FAILED (reallocation after free failed)\n");
        return 0;
    }
    *p5 = 555;
    if (*p5 != 555) {
        print("FAILED (data corruption after reuse)\n");
        return 0;
    }
    print("OK\n");

    // TEST 4: Array allocation
    print("TEST 4: Array allocation... ");
    int* arr = (int*)malloc(10 * sizeof(int));
    if (!arr) {
        print("FAILED (array allocation failed)\n");
        return 0;
    }
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }
    for (int i = 0; i < 10; i++) {
        if (arr[i] != i * 10) {
            print("FAILED (array data corruption)\n");
            return 0;
        }
    }
    print("OK\n");

    // TEST 5: Free array
    print("TEST 5: Free array... ");
    free(arr);
    print("OK\n");

    // TEST 6: Coalescing test
    print("TEST 6: Coalescing... ");
    int* a = (int*)malloc(100);
    int* b = (int*)malloc(100);
    int* c = (int*)malloc(100);
    free(a);
    free(b);
    free(c);
    int* big = (int*)malloc(500);  // Should reuse coalesced block
    if (!big) {
        print("FAILED (coalescing didn't work)\n");
        return 0;
    }
    free(big);
    print("OK\n");

    // TEST 7: NULL free (should not crash)
    print("TEST 7: NULL free... ");
    free(NULL);
    print("OK\n");

    // TEST 8: Stress test with many allocations
    print("TEST 8: Stress test ({d} allocations)... ", 100);
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = malloc(64 + (i % 32) * 4);  // Variable sizes
        if (!ptrs[i]) {
            print("FAILED (allocation {d} failed)\n", i);
            return 0;
        }
    }
    for (int i = 0; i < 100; i++) {
        free(ptrs[i]);
    }
    print("OK\n");

    // TEST 9: Large allocation
    print("TEST 9: Large allocation (10KB)... ");
    char* large = (char*)malloc(10240);
    if (!large) {
        print("FAILED (large allocation failed)\n");
        return 0;
    }
    for (int i = 0; i < 10240; i++) {
        large[i] = (char)(i & 0xFF);
    }
    for (int i = 0; i < 10240; i++) {
        if (large[i] != (char)(i & 0xFF)) {
            print("FAILED (large allocation data corruption)\n");
            return 0;
        }
    }
    free(large);
    print("OK\n");

    // TEST 10: Alternating allocations and frees
    print("TEST 10: Fragmentation test... ");
    int* x = (int*)malloc(64);
    int* y = (int*)malloc(64);
    int* z = (int*)malloc(64);
    free(x);
    int* w = (int*)malloc(32);
    if (!w) {
        print("FAILED (fragmentation handling)\n");
        return 0;
    }
    free(y);
    free(z);
    free(w);
    print("OK\n");

    print("\n=== ALL MALLOC TESTS PASSED ===\n\n");
    return 1;
}