#ifndef HEAP_H
#define HEAP_H

#define KERNEL_HEAP_START 0xC0000000
#define KERNEL_HEAP_SIZE  (4 * 1024 * 1024)

#include <stdint.h>
#include <stddef.h>

void heap_init(void);
void* kmalloc(size_t size);

#endif
