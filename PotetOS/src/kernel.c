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
#include "fs_api.h"

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

	/* Path System Tests */
	print("\n--- Path System Tests ---\n");

	/* Test 1: Parse absolute path */
	path_t *parsed = fs_parse_path("/home/user/file.txt");
	if (parsed && parsed->is_absolute && parsed->depth == 3) {
		print("[TEST] Parse path '/home/user/file.txt': PASS ({d} components)\n", parsed->depth);
	} else {
		print("[TEST] Parse path: FAIL\n");
	}

	/* Test 2: Create nested file using path */
	int32_t file_inode_num = fs_create_file("/home/user/config.txt", file_perms);
	if (file_inode_num >= 0) {
		print("[TEST] Create file using path '/home/user/config.txt': PASS (inode {d})\n", file_inode_num);
	} else {
		print("[TEST] Create file using path: FAIL\n");
	}

	/* Test 3: Find file by path */
	inode_t *found_file = fs_find("/home/user/config.txt");
	if (found_file && found_file->type == FILE_TYPE_REGULAR) {
		print("[TEST] Find file by path: PASS (inode {d})\n", found_file->inode_number);
	} else {
		print("[TEST] Find file by path: FAIL\n");
	}

	/* Test 4: Create another nested directory using path */
	int32_t logs_dir = fs_create_dir("/home/user/logs", test_perms);
	if (logs_dir >= 0) {
		print("[TEST] Create nested directory '/home/user/logs': PASS (inode {d})\n", logs_dir);
	} else {
		print("[TEST] Create nested directory: FAIL\n");
	}

	/* Test 5: Traverse existing path */
	inode_t *traversed = fs_traverse_path("/home/user/logs");
	if (traversed && traversed->type == FILE_TYPE_DIRECTORY) {
		print("[TEST] Traverse path '/home/user/logs': PASS (inode {d})\n", traversed->inode_number);
	} else {
		print("[TEST] Traverse path: FAIL\n");
	}

	/* Test 6: Try to find non-existent path */
	not_found = fs_find("/home/nonexistent");
	if (not_found == NULL) {
		print("[TEST] Find non-existent path: PASS (correctly returned NULL)\n");
	} else {
		print("[TEST] Find non-existent path: FAIL\n");
	}

	/* Test 7: Create file in nested directory */
	int32_t log_file_inode = fs_create_file("/home/user/logs/system.log", file_perms);
	if (log_file_inode >= 0) {
		print("[TEST] Create file in nested directory: PASS (inode {d})\n", log_file_inode);
	} else {
		print("[TEST] Create file in nested directory: FAIL\n");
	}

	/* Test 8: Remove file using path */
	int32_t rm_result = fs_remove_file("/home/user/config.txt");
	if (rm_result == 0) {
		print("[TEST] Remove file using path: PASS\n");
	} else {
		print("[TEST] Remove file using path: FAIL\n");
	}

	/* Test 9: Verify file was deleted */
	inode_t *deleted_check = fs_find("/home/user/config.txt");
	if (deleted_check == NULL) {
		print("[TEST] Verify file deletion: PASS (file no longer found)\n");
	} else {
		print("[TEST] Verify file deletion: FAIL\n");
	}

	print("--- Path System Tests Complete ---\n\n");

	/* File System API Tests */
	print("\n--- File System API Tests ---\n");

	/* Test 1: Check if file exists using API */
	bool exists = fs_api_exists("/home/user/logs/system.log");
	if (exists) {
		print("[TEST] File exists check: PASS\n");
	} else {
		print("[TEST] File exists check: FAIL\n");
	}

	/* Test 2: Check if path is a directory */
	bool is_dir = fs_api_is_directory("/home/user");
	if (is_dir) {
		print("[TEST] Is directory check: PASS\n");
	} else {
		print("[TEST] Is directory check: FAIL\n");
	}

	/* Test 3: Check if path is a file */
	bool is_file = fs_api_is_file("/home/user/logs/system.log");
	if (is_file) {
		print("[TEST] Is file check: PASS\n");
	} else {
		print("[TEST] Is file check: FAIL\n");
	}

	/* Test 4: Get file stats */
	fs_stat_t stat;
	int32_t stat_result = fs_api_stat("/home/user/logs/system.log", &stat);
	if (stat_result == FS_OK) {
		print("[TEST] Get file stats: PASS (inode {d}, type {d})\n", 
			stat.inode_number, stat.type);
	} else {
		print("[TEST] Get file stats: FAIL ({s})\n", fs_api_strerror(stat_result));
	}

	/* Test 5: List directory contents using API */
	fs_dirent_t entries[8];
	int32_t list_count = fs_api_listdir("/home/user", entries, 8);
	if (list_count >= 0) {
		print("[TEST] List directory: PASS ({d} entries)\n", list_count);
		for (int32_t i = 0; i < list_count; i++) {
			print("  - {s} (type {d})\n", entries[i].name, entries[i].type);
		}
	} else {
		print("[TEST] List directory: FAIL ({s})\n", fs_api_strerror(list_count));
	}

	/* Test 6: Try to create file that already exists */
	int32_t dup_result = fs_api_mkdir("/home/user");
	if (dup_result == FS_ERR_EXISTS) {
		print("[TEST] Duplicate creation prevention: PASS (correctly rejected)\n");
	} else {
		print("[TEST] Duplicate creation prevention: FAIL\n");
	}

	/* Test 7: Get error message */
	const char *err_msg = fs_api_strerror(FS_ERR_NOT_FOUND);
	if (err_msg) {
		print("[TEST] Error message lookup: PASS ({s})\n", err_msg);
	} else {
		print("[TEST] Error message lookup: FAIL\n");
	}

	/* Test 8: Create new directory with API */
	int32_t new_dir = fs_api_mkdir("/home/user/documents");
	if (new_dir == FS_OK) {
		print("[TEST] Create directory with API: PASS\n");
	} else {
		print("[TEST] Create directory with API: FAIL ({s})\n", fs_api_strerror(new_dir));
	}

	/* Test 9: Remove file with API */
	int32_t rm_api = fs_api_remove("/home/user/logs/system.log");
	if (rm_api == FS_OK) {
		print("[TEST] Remove file with API: PASS\n");
	} else {
		print("[TEST] Remove file with API: FAIL ({s})\n", fs_api_strerror(rm_api));
	}

	/* Test 10: Check removed file is gone */
	bool removed_check = fs_api_exists("/home/user/logs/system.log");
	if (!removed_check) {
		print("[TEST] Verify file removal: PASS (file no longer exists)\n");
	} else {
		print("[TEST] Verify file removal: FAIL\n");
	}

	print("--- File System API Tests Complete ---\n\n");

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
