#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "keyboard.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "print.h"
#include "multiboot.h"
#include "paging.h"
#include "pmm.h"
#include "vmm.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#define TEST_VIRT 0x400000  // 4MB

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


	/* Initialize the PMM */
	pmm_init(mbd->mmap_addr, mbd->mmap_length, 0);

	pmm_dump_stats();

	print("PMM init...OK!\n");


	/* Initialize Paging */
	init_paging();

	vmm_init();
	
	print("Paging init...OK!\n\n");
	
	print("Available Memory Map:\n");
	/* Make sure the magic number matches for memory mapping*/
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print("invalid magic number!\n");
    }

    /* Check bit 6 to see if we have a valid memory map */
    if(!(mbd->flags >> 6 & 0x1)) {
        print("invalid memory map given by GRUB bootloader\n");
    }

    /* Loop through the memory map and display the values */
    uint32_t mmap_end = mbd->mmap_addr + mbd->mmap_length;

	for (multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) mbd->mmap_addr;
		(uint32_t)mmmt < mmap_end;
		mmmt = (multiboot_memory_map_t*)((uint32_t)mmmt + mmmt->size + sizeof(mmmt->size)))
	{
		print("Start: {d}, Len: {d}, Size: {d}, Type: {d}\n",
			(int)mmmt->addr, (int)mmmt->len, mmmt->size, mmmt->type);
	}

	/* Different Paging Tests! */

	uint32_t *ptr = (uint32_t*)0x1000;
	*ptr = 0xDEADBEEF;

	if (*ptr == 0xDEADBEEF) {
		print("Paging OK: identity map works\n");
	} else {
		print("Paging BROKEN\n");
	}

	uint32_t a = pmm_alloc_frame();
	uint32_t b = pmm_alloc_frame();

	print("Allocated frames:\n");
	print("a: ");
	print_hex(a);
	print("\nb: ");
	print_hex(b);
	print("\n");

	uint32_t phys = pmm_alloc_frame();
	map_page(TEST_VIRT, phys, 0x3); // present | rw

	uint32_t *v = (uint32_t*)TEST_VIRT;
	*v = 0xCAFEBABE;

	if (*v == 0xCAFEBABE) {
		print("Virtual mapping OK\n");
	}

	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	// uint8_t mask = inb(PIC1_DATA);
	// terminal_writestring("PIC1 mask: ");
	// terminal_putchar('0' + mask);

	__asm__ volatile("sti"); // Enable interrupts


	print("--------------------------------------------------------------------------------");


	// Keep CPU running and wait for interrupts
	while (1) {
		asm volatile ("hlt");
	}
}
