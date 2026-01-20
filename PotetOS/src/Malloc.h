#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// MALLOC HEADER STRUCTURE
// ============================================================================

// Block header for tracking allocated/free memory chunks
// Placed immediately before the user data
typedef struct malloc_header {
    size_t size;                      // Size of user data (excluding header)
    struct malloc_header* next;       // Pointer to next block
    struct malloc_header* prev;       // Pointer to previous block
    bool free;                        // 1 if free, 0 if allocated
} malloc_header;

// ============================================================================
// MALLOC INTERFACE
// ============================================================================

// Initialize the malloc system
// Must be called once during kernel startup
void malloc_init(void);

// Allocate 'size' bytes of memory
// Returns pointer to allocated memory, or NULL on failure
void* malloc(size_t size);

// Deallocate previously allocated memory
// Safe to call with NULL pointer
void free(void* ptr);


// ============================================================================
// DEBUG & TESTING
// ============================================================================

// Print malloc statistics (useful for debugging fragmentation)
void malloc_dump_stats(void);

// Run comprehensive malloc tests
// Returns 1 if all tests pass, 0 if any test fails
int malloc_test(void);

#endif