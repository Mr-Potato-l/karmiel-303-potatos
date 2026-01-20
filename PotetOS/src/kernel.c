#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "keyboard.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "pit.h"
#include "print.h"
#include "multiboot.h"
#include "paging.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "Malloc.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#define TEST_VIRT 0x400000  // 4MB

static void task_a(void) {
	while (1) {
		print("Task A running\n");
		/* simple busy delay */
		for (volatile uint32_t i = 0; i < 200000; i++);
		scheduler_yield();
	}
}

static void task_b(void) {
	while (1) {
		print("Task B running\n");
		for (volatile uint32_t i = 0; i < 200000; i++);
		scheduler_yield();
	}
}

void kernel_main(multiboot_info_t* mbd, uint32_t magic)
{
	// Initialize terminal interface
	terminal_initialize();
	print("Terminal init...OK!\n");
	
	
	/* Initialize the GDT */
	gdt_install();
	
	print("GDT init...OK!\n");
	
	
	/* Initialize the IDT */
	idt_install();
	
	print("IDT init...OK!\n");

	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	/* Initialize PIT for 100Hz timer */
	pit_init(100);

	/* Initialize the PMM */
	pmm_init(mbd->mmap_addr, mbd->mmap_length, 0);

	pmm_dump_stats();

	print("PMM init...OK!\n");


	/* Initialize Paging */
	init_paging();

	vmm_init();
	print("Paging init...OK!\n\n");


	heap_init();
	print("Heap init...OK!\n\n");


	malloc_init();
	print("Malloc init...OK!\n\n");
	
	// ========== MALLOC TESTING ==========
	// Uncomment the lines below to run malloc tests
	
	// Run comprehensive test suite
	malloc_test();
	
	// Dump statistics to see heap state
	malloc_dump_stats();
	
	// ====================================
	
	
	// print("Available Memory Map:\n");
	// /* Make sure the magic number matches for memory mapping*/
    // if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    //     print("invalid magic number!\n");
    // }

    // /* Check bit 6 to see if we have a valid memory map */
    // if(!(mbd->flags >> 6 & 0x1)) {
    //     print("invalid memory map given by GRUB bootloader\n");
    // }
	//
    // /* Loop through the memory map and display the values */
    // uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

	// for (multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) mbd->mmap_addr;
	// 	(uint32_t)mmmt < mmap_end;
	// 	mmmt = (multiboot_memory_map_t*)((uint32_t)mmmt + mmmt->size + sizeof(mmmt->size)))
	// {
	// 	// uint32_t addr_lo = (uint32_t)(mmmt->addr & 0xFFFFFFFF);
	// 	// uint32_t addr_hi = (uint32_t)(mmmt->addr >> 32);

	// 	// uint32_t len_lo  = (uint32_t)(mmmt->len & 0xFFFFFFFF);
	// 	// uint32_t len_hi  = (uint32_t)(mmmt->len >> 32);

	// 	print("Start: {a}, Len: {a}, Size: {d}, Type: {d}\n",
	// 		mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
	// }

	/* Different Paging Tests! */

	uint32_t *ptr = (uint32_t*)0x1000;
	*ptr = 0xDEADBEEF;

	if (*ptr == 0xDEADBEEF) {
		print("Paging OK: identity map works\n");
	} else {
		print("Paging BROKEN\n");
	}

	// uint32_t frame_a = pmm_alloc_frame();
	// uint32_t frame_b = pmm_alloc_frame();

	// print("Allocated frames:\n");
	// print("a: ");
	// print_hex(frame_a);
	// print("\nb: ");
	// print_hex(frame_b);
	// print("\n");

	uint32_t phys = pmm_alloc_frame();
	map_page(TEST_VIRT, phys, 0x3); // present | rw

	uint32_t *v = (uint32_t*)TEST_VIRT;
	*v = 0xCAFEBABE;

	if (*v == 0xCAFEBABE) {
		print("Virtual mapping OK\n");
	}

	// int* a = kmalloc(sizeof(int));
	// int* b = kmalloc(sizeof(int));

	// *a = 1337;
	// *b = 0xDEADBEEF;

	// print("heap a = ");
	// print_hex(*a);
	// print("\nheap b = ");
	// print_hex(*b);
	// print("\n");


	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	// uint8_t mask = inb(PIC1_DATA);
	// print("PIC1 mask: ");
	// terminal_putchar('0' + mask);

	__asm__ volatile("sti"); // Enable interrupts

	scheduler_init();

	/* Create two simple tasks for demonstration */
	scheduler_create(task_a);
	scheduler_create(task_b);


	print("--------------------------------------------------------------------------------");


	// Keep CPU running and wait for interrupts. Print a message every 100 ticks.
	uint32_t last_ticks = pit_get_ticks();
	while (1) {
		uint32_t t = pit_get_ticks();
		if (t != last_ticks) {
			last_ticks = t;
			if (t % 100 == 0) {
				print("\nTicks: {d}\n", t);
			}
		}
		/* Run any pending scheduled tasks (cooperative) */
		scheduler_run_pending();
		asm volatile ("hlt");
	}
}
