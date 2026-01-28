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
#include "scheduler.h"
#include "filesystem.h"

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

	print("Heap init...OK!\n");

	
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

		print("Start: {a}, Len: {a}, Size: {d}, Type: {d}\n",
			mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
	}

	/* Different Paging Tests! */

	uint32_t *ptr = (uint32_t*)0x1000;
	*ptr = 0xDEADBEEF;

	if (*ptr == 0xDEADBEEF) {
		print("Paging OK: identity map works\n");
	} else {
		print("Paging BROKEN\n");
	}

	uint32_t frame_a = pmm_alloc_frame();
	uint32_t frame_b = pmm_alloc_frame();

	print("Allocated frames:\n");
	print("a: ");
	print_hex(frame_a);
	print("\nb: ");
	print_hex(frame_b);
	print("\n");

	uint32_t phys = pmm_alloc_frame();
	map_page(TEST_VIRT, phys, 0x3); // present | rw

	uint32_t *v = (uint32_t*)TEST_VIRT;
	*v = 0xCAFEBABE;

	if (*v == 0xCAFEBABE) {
		print("Virtual mapping OK\n");
	}

	int* a = kmalloc(sizeof(int));
	int* b = kmalloc(sizeof(int));

	*a = 1337;
	*b = 0xDEADBEEF;

	print("heap a = ");
	print_hex(*a);
	print("\nheap b = ");
	print_hex(*b);
	print("\n");


	/* File System Tests */
	print("\n--- File System Tests ---\n");
	fs_init();

	/* Test 1: Check root inode creation */
	inode_t *root = fs_inode_get(0);
	if (root && root->type == FILE_TYPE_DIRECTORY) {
		print("[TEST] Root inode created: PASS\n");
	} else {
		print("[TEST] Root inode creation: FAIL\n");
	}

	/* Test 2: Create a new regular file inode */
	file_perms_t file_perms = { .permissions = 0644, .uid = 0, .gid = 0 };
	inode_t *file_inode = fs_inode_create(FILE_TYPE_REGULAR, file_perms);
	if (file_inode) {
		print("[TEST] Regular file inode created (inode #{d}): PASS\n", file_inode->inode_number);
	} else {
		print("[TEST] Regular file inode creation: FAIL\n");
	}

	/* Test 3: Allocate blocks */
	uint32_t block1 = fs_allocate_block();
	uint32_t block2 = fs_allocate_block();
	if (block1 != 0xFFFFFFFF && block2 != 0xFFFFFFFF && block1 != block2) {
		print("[TEST] Block allocation: PASS (blocks {d}, {d})\n", block1, block2);
	} else {
		print("[TEST] Block allocation: FAIL\n");
	}

	/* Test 4: Get block address */
	void *block_addr = fs_get_block_address(block1);
	if (block_addr) {
		print("[TEST] Block address retrieval: PASS (addr: {x})\n", (uint32_t)block_addr);
	} else {
		print("[TEST] Block address retrieval: FAIL\n");
	}

	/* Test 5: File handle operations */
	int32_t handle = fs_open("testfile.txt", 0x01);  // Read flag
	if (handle >= 0) {
		print("[TEST] File open: PASS (handle {d})\n", handle);
		fs_close(handle);
		print("[TEST] File close: PASS\n");
	} else {
		print("[TEST] File open: FAIL\n");
	}

	/* Test 6: Free a block and verify */
	fs_free_block(block1);
	if (g_fs.free_blocks > 0) {
		print("[TEST] Block deallocation: PASS (free blocks: {d})\n", g_fs.free_blocks);
	} else {
		print("[TEST] Block deallocation: FAIL\n");
	}

	print("--- File System Tests Complete ---\n\n");

	/* Directory Tests */
	print("\n--- Directory Tests ---\n");

	/* Test 1: Create subdirectory in root */
	file_perms_t test_perms = { .permissions = 0755, .uid = 0, .gid = 0 };
	int32_t home_dir = fs_mkdir("home", g_fs.root_inode, test_perms);
	if (home_dir >= 0) {
		print("[TEST] Create /home directory: PASS (inode {d})\n", home_dir);
	} else {
		print("[TEST] Create /home directory: FAIL\n");
	}

	/* Test 2: Create another subdirectory */
	int32_t var_dir = fs_mkdir("var", g_fs.root_inode, test_perms);
	if (var_dir >= 0) {
		print("[TEST] Create /var directory: PASS (inode {d})\n", var_dir);
	} else {
		print("[TEST] Create /var directory: FAIL\n");
	}

	/* Test 3: Create nested subdirectory */
	inode_t *home_inode = fs_inode_get(home_dir);
	int32_t user_dir = fs_mkdir("user", home_inode, test_perms);
	if (user_dir >= 0) {
		print("[TEST] Create /home/user directory: PASS (inode {d})\n", user_dir);
	} else {
		print("[TEST] Create /home/user directory: FAIL\n");
	}

	/* Test 4: List root directory entries */
	uint32_t root_entry_count = 0;
	dir_entry_t *root_entries = fs_readdir(g_fs.root_inode, &root_entry_count);
	if (root_entries && root_entry_count == 2) {
		print("[TEST] List root directory: PASS ({d} entries)\n", root_entry_count);
		for (uint32_t i = 0; i < root_entry_count; i++) {
			print("  - {s} (inode {d})\n", 
				root_entries[i].filename, root_entries[i].inode_number);
		}
	} else {
		print("[TEST] List root directory: FAIL (got {d} entries, expected 2)\n", root_entry_count);
	}

	/* Test 5: List /home directory entries */
	uint32_t home_entry_count = 0;
	dir_entry_t *home_entries = fs_readdir(home_inode, &home_entry_count);
	if (home_entries && home_entry_count == 1) {
		print("[TEST] List /home directory: PASS ({d} entry)\n", home_entry_count);
		print("  - {s} (inode {d})\n", 
			home_entries[0].filename, home_entries[0].inode_number);
	} else {
		print("[TEST] List /home directory: FAIL (got {d} entries, expected 1)\n", home_entry_count);
	}

	/* Test 6: Find file by name in directory */
	inode_t *found = fs_find_in_dir(g_fs.root_inode, "home");
	if (found && found->inode_number == home_dir) {
		print("[TEST] Find 'home' in root: PASS (inode {d})\n", found->inode_number);
	} else {
		print("[TEST] Find 'home' in root: FAIL\n");
	}

	/* Test 7: Find non-existent file */
	inode_t *not_found = fs_find_in_dir(g_fs.root_inode, "nonexistent");
	if (not_found == NULL) {
		print("[TEST] Find non-existent file: PASS (correctly returned NULL)\n");
	} else {
		print("[TEST] Find non-existent file: FAIL\n");
	}

	/* Test 8: Remove empty directory */
	int32_t tmp_dir = fs_mkdir("tmp", g_fs.root_inode, test_perms);
	if (tmp_dir >= 0) {
		print("[TEST] Create /tmp for removal test: PASS (inode {d})\n", tmp_dir);
		int32_t rm_result = fs_rmdir("tmp", g_fs.root_inode);
		if (rm_result == 0) {
			print("[TEST] Remove /tmp directory: PASS\n");
		} else {
			print("[TEST] Remove /tmp directory: FAIL\n");
		}
	}

	/* Test 9: Try to remove non-empty directory (should fail) */
	int32_t rm_home = fs_rmdir("home", g_fs.root_inode);
	if (rm_home != 0) {
		print("[TEST] Prevent removal of non-empty directory: PASS (correctly rejected)\n");
	} else {
		print("[TEST] Prevent removal of non-empty directory: FAIL\n");
	}

	/* Test 10: Check root directory has correct number of entries after tests */
	uint32_t final_count = 0;
	fs_readdir(g_fs.root_inode, &final_count);
	if (final_count == 2) {
		print("[TEST] Final root directory count: PASS ({d} entries)\n", final_count);
	} else {
		print("[TEST] Final root directory count: FAIL (got {d}, expected 2)\n", final_count);
	}

	print("--- Directory Tests Complete ---\n\n");

	IRQ_clear_mask(0);
	IRQ_clear_mask(1); // Clear mask on keyboard IRQ line

	__asm__ volatile("sti"); // Enable interrupts

	scheduler_init();

	print("--------------------------------------------------------------------------------");


	// Keep CPU running and wait for interrupts.
	while (1) {
		/* Run any pending scheduled tasks (cooperative) */
		scheduler_run_pending();
		asm volatile ("hlt");
	}
}
