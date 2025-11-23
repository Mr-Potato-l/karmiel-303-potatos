#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "gdt.h"
#include "idt.h"
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

	terminal_writestring("Terminal init...OK!\n");
	

	/* Initialize the GDT */
	gdt_install();
	
	terminal_writestring("GDT init...OK!\n");


	/* Initialize the IDT */
	idt_install();

	terminal_writestring("IDT init...OK!\n");


	char ex = 'Y';
	int num = -5;
	char* str = "Hello, World!";
	float fnum = 3.14; 
	
	print("char print: {c}\n", ex);
	print("int print: {d}\n", num);
	print("string print: {s}\n", str);
	print("float print: {f}\n", fnum);
	print("hex print: {x}\n", 305441741);

	
	terminal_writestring("Testing first interrupt!\n");

	__asm__("xor %eax, %eax");
	__asm__("div %eax");

	terminal_writestring("If you see this message, the interrupt handling failed!\n");
}
