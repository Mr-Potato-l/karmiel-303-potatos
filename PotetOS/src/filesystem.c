#include "filesystem.h"
#include "heap.h"
#include "print.h"

/* Forward declare memset since we can't use standard library */
extern void *memset(void *dst, int val, size_t count);

/* Global file system instance */
static filesystem_t g_fs;

/* File handle table */
static file_handle_t g_file_table[MAX_FILES_OPEN];

/**
 * Initialize the file system
 * Sets up in-memory structures for file management
 */
void fs_init(void)
{
    print("[FS] Initializing file system...\n");

    /* Initialize super block */
    g_fs.magic_number = 0xDEADBEEF;
    g_fs.block_size = BLOCK_SIZE;
    g_fs.inode_size = INODE_SIZE;
    g_fs.total_blocks = 1024;          /* Allocate space for 1024 blocks */
    g_fs.total_inodes = MAX_FILES_TOTAL;
    g_fs.free_blocks = g_fs.total_blocks;
    g_fs.free_inodes = g_fs.total_inodes;

    /* Calculate bitmap sizes (in bytes) */
    g_fs.block_bitmap_size = (g_fs.total_blocks + 7) / 8;
    g_fs.inode_bitmap_size = (g_fs.total_inodes + 7) / 8;

    /* Allocate bitmaps from heap */
    g_fs.block_bitmap = (uint32_t *)kmalloc(g_fs.block_bitmap_size);
    g_fs.inode_bitmap = (uint32_t *)kmalloc(g_fs.inode_bitmap_size);

    /* Allocate inode table from heap */
    g_fs.inode_table = (inode_t *)kmalloc(sizeof(inode_t) * g_fs.total_inodes);

    if (!g_fs.block_bitmap || !g_fs.inode_bitmap || !g_fs.inode_table) {
        print("[FS] ERROR: Failed to allocate memory for file system structures\n");
        return;
    }

    /* Initialize bitmaps (0 = free, 1 = used) */
    memset(g_fs.block_bitmap, 0, g_fs.block_bitmap_size);
    memset(g_fs.inode_bitmap, 0, g_fs.inode_bitmap_size);
    memset(g_fs.inode_table, 0, sizeof(inode_t) * g_fs.total_inodes);

    /* Initialize file handle table */
    for (int i = 0; i < MAX_FILES_OPEN; i++) {
        g_file_table[i].is_open = false;
        g_file_table[i].inode_number = 0;
        g_file_table[i].offset = 0;
        g_file_table[i].flags = 0;
    }

    /* Create root directory inode */
    file_perms_t root_perms = {
        .permissions = 0755,
        .uid = 0,
        .gid = 0
    };
    g_fs.root_inode = fs_inode_create(FILE_TYPE_DIRECTORY, root_perms);

    if (g_fs.root_inode) {
        g_fs.root_inode->inode_number = 0;
        print("[FS] File system initialized successfully\n");
        print("[FS] Root inode created at address: 0x%x\n", (uint32_t)g_fs.root_inode);
        print("[FS] Total blocks: %u, Total inodes: %u\n", 
              g_fs.total_blocks, g_fs.total_inodes);
    } else {
        print("[FS] ERROR: Failed to create root inode\n");
    }
}

/**
 * Shutdown the file system
 */
void fs_shutdown(void)
{
    print("[FS] Shutting down file system...\n");
    /* In a real filesystem, we would flush all changes to disk here */
    /* For now, memory will be cleaned up when kernel exits */
}

/**
 * Allocate and create a new inode
 */
inode_t* fs_inode_create(file_type_t type, file_perms_t perms)
{
    uint32_t inode_num = fs_get_free_inode();
    if (inode_num >= g_fs.total_inodes) {
        print("[FS] ERROR: No free inodes available\n");
        return NULL;
    }

    /* Get inode from table */
    inode_t *inode = &g_fs.inode_table[inode_num];
    memset(inode, 0, sizeof(inode_t));

    /* Initialize inode */
    inode->inode_number = inode_num;
    inode->type = type;
    inode->size = 0;
    inode->block_count = 0;
    inode->blocks = (uint32_t *)kmalloc(sizeof(uint32_t) * 256);  /* Support up to 256 blocks */
    inode->link_count = 1;
    inode->perms = perms;

    if (!inode->blocks) {
        print("[FS] ERROR: Failed to allocate block pointer array for inode\n");
        return NULL;
    }

    memset(inode->blocks, 0, sizeof(uint32_t) * 256);

    /* Mark inode as used in bitmap */
    fs_set_inode_bitmap(inode_num, true);
    g_fs.free_inodes--;

    return inode;
}

/**
 * Delete an inode and free its blocks
 */
void fs_inode_delete(uint32_t inode_number)
{
    if (inode_number >= g_fs.total_inodes) {
        print("[FS] ERROR: Invalid inode number\n");
        return;
    }

    inode_t *inode = &g_fs.inode_table[inode_number];

    /* Free all data blocks */
    for (uint32_t i = 0; i < inode->block_count; i++) {
        fs_free_block(inode->blocks[i]);
    }

    /* Free the block pointer array */
    /* Note: In a real implementation, we'd use a memory manager to track this */

    /* Mark inode as free */
    fs_set_inode_bitmap(inode_number, false);
    g_fs.free_inodes++;

    memset(inode, 0, sizeof(inode_t));
}

/**
 * Get an inode by number
 */
inode_t* fs_inode_get(uint32_t inode_number)
{
    if (inode_number >= g_fs.total_inodes) {
        return NULL;
    }

    inode_t *inode = &g_fs.inode_table[inode_number];
    if (inode->inode_number == inode_number && inode->inode_number != 0) {
        return inode;
    }

    /* Return if it's the root inode */
    if (inode_number == 0 && g_fs.root_inode != NULL) {
        return g_fs.root_inode;
    }

    return NULL;
}

/**
 * Allocate a free block
 */
uint32_t fs_allocate_block(void)
{
    if (g_fs.free_blocks == 0) {
        print("[FS] ERROR: No free blocks available\n");
        return 0xFFFFFFFF;
    }

    /* Find first free block */
    uint8_t *bitmap = (uint8_t *)g_fs.block_bitmap;
    for (uint32_t i = 0; i < g_fs.total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;

        if (!(bitmap[byte_idx] & (1 << bit_idx))) {
            /* Found free block */
            fs_set_block_bitmap(i, true);
            g_fs.free_blocks--;
            return i;
        }
    }

    print("[FS] ERROR: Block allocation failed\n");
    return 0xFFFFFFFF;
}

/**
 * Free a block
 */
void fs_free_block(uint32_t block_number)
{
    if (block_number >= g_fs.total_blocks) {
        print("[FS] ERROR: Invalid block number: %u\n", block_number);
        return;
    }

    fs_set_block_bitmap(block_number, false);
    g_fs.free_blocks++;
}

/**
 * Get the memory address of a block
 */
void* fs_get_block_address(uint32_t block_number)
{
    if (block_number >= g_fs.total_blocks) {
        return NULL;
    }

    /* Calculate block address in the kernel heap */
    uint32_t heap_offset = block_number * BLOCK_SIZE;
    return (void *)(KERNEL_HEAP_START + heap_offset);
}

/**
 * Find the first free inode
 */
uint32_t fs_get_free_inode(void)
{
    uint8_t *bitmap = (uint8_t *)g_fs.inode_bitmap;
    for (uint32_t i = 0; i < g_fs.total_inodes; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;

        if (!(bitmap[byte_idx] & (1 << bit_idx))) {
            return i;
        }
    }

    return g_fs.total_inodes;  /* No free inode */
}

/**
 * Set inode bitmap bit
 */
void fs_set_inode_bitmap(uint32_t inode_num, bool used)
{
    if (inode_num >= g_fs.total_inodes) {
        return;
    }

    uint8_t *bitmap = (uint8_t *)g_fs.inode_bitmap;
    uint32_t byte_idx = inode_num / 8;
    uint32_t bit_idx = inode_num % 8;

    if (used) {
        bitmap[byte_idx] |= (1 << bit_idx);
    } else {
        bitmap[byte_idx] &= ~(1 << bit_idx);
    }
}

/**
 * Set block bitmap bit
 */
void fs_set_block_bitmap(uint32_t block_num, bool used)
{
    if (block_num >= g_fs.total_blocks) {
        return;
    }

    uint8_t *bitmap = (uint8_t *)g_fs.block_bitmap;
    uint32_t byte_idx = block_num / 8;
    uint32_t bit_idx = block_num % 8;

    if (used) {
        bitmap[byte_idx] |= (1 << bit_idx);
    } else {
        bitmap[byte_idx] &= ~(1 << bit_idx);
    }
}

/**
 * Open a file
 */
int32_t fs_open(const char *filename, uint8_t flags)
{
    /* Find free file handle */
    for (int i = 0; i < MAX_FILES_OPEN; i++) {
        if (!g_file_table[i].is_open) {
            g_file_table[i].is_open = true;
            g_file_table[i].offset = 0;
            g_file_table[i].flags = flags;
            return i;
        }
    }

    print("[FS] ERROR: No free file handles\n");
    return -1;
}

/**
 * Close a file
 */
void fs_close(int32_t handle)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN) {
        print("[FS] ERROR: Invalid file handle\n");
        return;
    }

    g_file_table[handle].is_open = false;
    g_file_table[handle].offset = 0;
    g_file_table[handle].inode_number = 0;
}

/**
 * Read from a file
 */
size_t fs_read(int32_t handle, void *buffer, size_t bytes)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !g_file_table[handle].is_open) {
        print("[FS] ERROR: Invalid file handle for read\n");
        return 0;
    }

    /* Placeholder implementation */
    memset(buffer, 0, bytes);
    return 0;
}

/**
 * Write to a file
 */
size_t fs_write(int32_t handle, const void *buffer, size_t bytes)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !g_file_table[handle].is_open) {
        print("[FS] ERROR: Invalid file handle for write\n");
        return 0;
    }

    /* Placeholder implementation */
    return 0;
}

/**
 * Seek in a file
 */
int32_t fs_seek(int32_t handle, int32_t offset, int whence)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !g_file_table[handle].is_open) {
        print("[FS] ERROR: Invalid file handle for seek\n");
        return -1;
    }

    /* Placeholder implementation */
    return 0;
}

/**
 * Create a directory
 */
int32_t fs_mkdir(const char *path, file_perms_t perms)
{
    inode_t *dir_inode = fs_inode_create(FILE_TYPE_DIRECTORY, perms);
    if (!dir_inode) {
        print("[FS] ERROR: Failed to create directory inode\n");
        return -1;
    }

    return dir_inode->inode_number;
}

/**
 * Remove a directory
 */
int32_t fs_rmdir(const char *path)
{
    /* Placeholder implementation */
    return 0;
}

/**
 * Read directory entries
 */
dir_entry_t* fs_readdir(inode_t *dir_inode)
{
    if (!dir_inode || dir_inode->type != FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    /* Placeholder implementation */
    return NULL;
}
