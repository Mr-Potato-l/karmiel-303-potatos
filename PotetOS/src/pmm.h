#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PMM_BITMAP_MAX_FRAMES 32768   // 128MB
static uint32_t frames_bitmap_static[PMM_BITMAP_MAX_FRAMES / 32];


/* Initialize the physical memory manager from a multiboot memory map.
 * mmap_addr: pointer (physical) to multiboot mmap entries (as GRUB gives you)
 * mmap_length: length in bytes of the mmap buffer
 * max_phys: optional limit of physical memory you want to manage (0 = use map max)
 */
void pmm_init(uint32_t mmap_addr, uint32_t mmap_length, uint32_t max_phys);

/* Allocate and free single 4KiB frames.
 * Returns physical address of frame (4K aligned), or 0 if no frame available.
 */
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys_addr);

/* Mark a physical range used or free (phys_addr must be page-aligned) */
void pmm_mark_used(uint32_t phys_addr, uint32_t size);
void pmm_mark_free(uint32_t phys_addr, uint32_t size);

/* Helpers */
uint32_t pmm_total_frames(void);
uint32_t pmm_total_free_frames(void);

/* Debug helper (optional) */
void pmm_dump_stats(void);

#endif
