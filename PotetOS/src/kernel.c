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
#include "Scanf.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void initializer(multiboot_info_t* mbd);

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
	initializer(mbd);

	print("PotetOS Kernel Initialized!\n");

	char* x = "empty";
	int tst = 0;
	print("before scanf: x = {s},tst = {d}\n", x, tst);
	scanf("{s}{d}", x, &tst);
	print("after scanf: x = {s},tst = {d}\n", x, tst);



	// // Keep CPU running and wait for interrupts
	// while (1) {
	// 	asm volatile ("hlt");
	// }
}

// Kernel initialization routine
void initializer(multiboot_info_t* mbd){
	// Initialize terminal interface
	terminal_initialize();
	
	/* Initialize the GDT */
	gdt_install();
	
	/* Initialize the IDT */
	idt_install();
	
	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	/* Initialize PIT for 100Hz timer */
	pit_init(100);

	/* Initialize the PMM */
	pmm_init(mbd->mmap_addr, mbd->mmap_length, 0);

	/* Initialize Paging */
	init_paging();

	vmm_init();

	heap_init();
	print("Heap init...OK!\n\n");

	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	__asm__ volatile("sti"); // Enable interrupts
}