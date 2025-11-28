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

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main(multiboot_info_t* mbd, uint32_t magic)
{
	// Initialize terminal interface
	terminal_initialize();
	print("Terminal init...OK!\n");

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
			mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
	}


	/* Initialize the GDT */
	gdt_install();
	
	print("GDT init...OK!\n");


	/* Initialize the IDT */
	idt_install();

	print("IDT init...OK!\n");


	/* Initialize Paging */
	init_paging();

	print("Paging init...OK!\n\n");


	// Testing Paging
	volatile uint32_t* p = (uint32_t*)0xDEADBEEF;
	uint32_t x = *p;


	// Testing print function

	char ex = 'Y';
	int num = -5;
	char* str = "Hello, World!";
	float fnum = 3.14; 
	
	print("char print: {c}\n", ex);
	print("int print: {d}\n", num);
	print("string print: {s}\n", str);
	print("float print: {f}\n", fnum);
	print("hex print: {x}\n\n", 305441741);

	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	// uint8_t mask = inb(PIC1_DATA);
	// terminal_writestring("PIC1 mask: ");
	// terminal_putchar('0' + mask);

	__asm__ volatile("sti"); // Enable interrupts


	// terminal_writestring("Testing first interrupt!\n");

	// __asm__("xor %eax, %eax");
	// __asm__("div %eax");

	// terminal_writestring("If you see this message, the interrupt handling failed!\n");

	print("\nTesting keyboard:\n");

	// __asm__ volatile("int $33");


	// Keep CPU running and wait for interrupts
	while (1) {
		asm volatile ("hlt");
	}
}
