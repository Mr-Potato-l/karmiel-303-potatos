#include "pmm.h"
#include <stdint.h>
#include <string.h>

#define PAGE_SIZE 0x1000u


typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;


static uint32_t *frames_bitmap = 0;
static uint32_t frames_bitmap_u32 = 0;
static uint32_t total_frames = 0;
static uint32_t managed_phys_bytes = 0;


#define INDEX_FROM_BIT(a) ((a) / 32U)
#define OFFSET_FROM_BIT(a) ((a) % 32U)


static inline void bitmap_set(uint32_t frame_idx) {
    frames_bitmap[INDEX_FROM_BIT(frame_idx)] |= (1U << OFFSET_FROM_BIT(frame_idx));
}

static inline void bitmap_clear(uint32_t frame_idx) {
    frames_bitmap[INDEX_FROM_BIT(frame_idx)] &= ~(1U << OFFSET_FROM_BIT(frame_idx));
}

static inline int bitmap_test(uint32_t frame_idx) {
    return (frames_bitmap[INDEX_FROM_BIT(frame_idx)] & (1U << OFFSET_FROM_BIT(frame_idx))) != 0;
}

static uint32_t find_first_free_frame(void) {
    for (uint32_t i = 0; i < frames_bitmap_u32; ++i) {
        if (frames_bitmap[i] != 0xFFFFFFFFU) {
            for (int bit = 0; bit < 32; ++bit) {
                uint32_t mask = 1U << bit;
                if (!(frames_bitmap[i] & mask)) {
                    uint32_t frame = i * 32U + bit;
                    if (frame < total_frames) return frame;
                    return (uint32_t)-1;
                }
            }
        }
    }
    return (uint32_t)-1;
}

static inline uint32_t frame_to_phys(uint32_t frame_idx) {
    return frame_idx * PAGE_SIZE;
}
static inline uint32_t phys_to_frame(uint32_t phys) {
    return phys / PAGE_SIZE;
}

void pmm_mark_used(uint32_t phys_addr, uint32_t size) {
    if (size == 0) return;
    uint32_t start = phys_to_frame(phys_addr & ~(PAGE_SIZE - 1));
    uint32_t end = phys_to_frame((phys_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    for (uint32_t f = start; f < end && f < total_frames; ++f) {
        bitmap_set(f);
    }
}

void pmm_mark_free(uint32_t phys_addr, uint32_t size) {
    if (size == 0) return;
    uint32_t start = phys_to_frame(phys_addr & ~(PAGE_SIZE - 1));
    uint32_t end = phys_to_frame((phys_addr + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    for (uint32_t f = start; f < end && f < total_frames; ++f) {
        bitmap_clear(f);
    }
}

uint32_t pmm_alloc_frame(void) {
    uint32_t fi = find_first_free_frame();
    if (fi == (uint32_t)-1) return 0;
    bitmap_set(fi);
    return frame_to_phys(fi);
}

void pmm_free_frame(uint32_t phys_addr) {
    uint32_t fi = phys_to_frame(phys_addr);
    if (fi >= total_frames) return;
    bitmap_clear(fi);
}

void pmm_init(uint32_t mmap_addr, uint32_t mmap_length, uint32_t max_phys) {
    uint32_t highest = 0;
    uint32_t pos = 0;

    while (pos < mmap_length) {
        multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)(uintptr_t)(mmap_addr + pos);
        if ((uint32_t)(e->addr + e->len) > highest) {
            uint64_t end = e->addr + e->len;
            if (end > 0xFFFFFFFFULL) end = 0xFFFFFFFFULL;
            if ((uint32_t)end > highest) highest = (uint32_t)end;
        }
        pos += e->size + sizeof(e->size);
    }

    if (max_phys && max_phys < highest) highest = max_phys;
    managed_phys_bytes = highest;
    total_frames = (managed_phys_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    frames_bitmap_u32 = (total_frames + 31) / 32;

    

    extern uint32_t page_directory[];
    uint8_t *after_pd = (uint8_t*)page_directory + 4096;
    frames_bitmap = (uint32_t*)after_pd;

    for (uint32_t i = 0; i < frames_bitmap_u32; ++i) frames_bitmap[i] = 0;

    for (uint32_t f = 0; f < total_frames; ++f) bitmap_set(f);

    pos = 0;
    while (pos < mmap_length) {
        multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)(uintptr_t)(mmap_addr + pos);
        if (e->type == 1) {
            uint32_t start = (uint32_t)(e->addr & 0xFFFFFFFFULL);
            uint32_t len = (uint32_t)(e->len & 0xFFFFFFFFULL);
            uint32_t s_frame = phys_to_frame(start & ~(PAGE_SIZE - 1));
            uint32_t e_frame = phys_to_frame((start + len + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
            for (uint32_t f = s_frame; f < e_frame && f < total_frames; ++f) {
                bitmap_clear(f);
            }
        }
        pos += e->size + sizeof(e->size);
    }

    uint32_t pd_phys = (uint32_t)(uintptr_t)page_directory;
    pmm_mark_used(pd_phys, 4096 + frames_bitmap_u32 * 4);
}

uint32_t pmm_total_frames(void) { return total_frames; }

uint32_t pmm_total_free_frames(void) {
    uint32_t free = 0;
    for (uint32_t f = 0; f < total_frames; ++f) if (!bitmap_test(f)) ++free;
    return free;
}

void pmm_dump_stats(void) {
    extern void terminal_writestring(const char*);
    extern char* inttoa(int, char*);
    char buf[64];
    terminal_writestring("PMM: total frames: ");
    inttoa((int)total_frames, buf);
    terminal_writestring(buf);
    terminal_writestring("\nPMM: free frames: ");
    inttoa((int)pmm_total_free_frames(), buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
}
