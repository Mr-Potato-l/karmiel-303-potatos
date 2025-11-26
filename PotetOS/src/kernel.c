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

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void kernel_main(void) 
{
	// Initialize terminal interface
	terminal_initialize();

	print("Terminal init...OK!\n");
	

	/* Initialize the GDT */
	gdt_install();
	
	print("GDT init...OK!\n");


	/* Initialize the IDT */
	idt_install();

	print("IDT init...OK!\n\n");


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
	// print("PIC1 mask: ");
	// terminal_putchar('0' + mask);

	__asm__ volatile("sti"); // Enable interrupts


	// print("Testing first interrupt!\n");

	// __asm__("xor %eax, %eax");
	// __asm__("div %eax");

	// print("If you see this message, the interrupt handling failed!\n");

	print("\nTesting keyboard:\n");

	// __asm__ volatile("int $33");


	// Keep CPU running and wait for interrupts
	while (1) {
		asm volatile ("hlt");
	}
}
