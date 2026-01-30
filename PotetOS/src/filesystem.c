#include "filesystem.h"
#include "heap.h"
#include "print.h"

/* Forward declare memset since we can't use standard library */
extern void *memset(void *dst, int val, size_t count);

/* Global file system instance */
filesystem_t g_fs;

/* File handle table */
static file_handle_t g_file_table[MAX_FILES_OPEN];

/* Static storage for bitmaps and inode table to avoid heap issues */
static uint8_t g_block_bitmap_storage[(MAX_BLOCKS + 7) / 8];
static uint8_t g_inode_bitmap_storage[(MAX_FILES_TOTAL + 7) / 8];
static inode_t g_inode_table_storage[MAX_FILES_TOTAL];

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
    g_fs.total_blocks = MAX_BLOCKS;
    g_fs.total_inodes = MAX_FILES_TOTAL;
    g_fs.free_blocks = g_fs.total_blocks;
    g_fs.free_inodes = g_fs.total_inodes;

    /* Calculate bitmap sizes (in bytes) */
    g_fs.block_bitmap_size = (g_fs.total_blocks + 7) / 8;
    g_fs.inode_bitmap_size = (g_fs.total_inodes + 7) / 8;

    /* Use static storage instead of heap allocation */
    print("[FS] Initializing block bitmap ({d} bytes)...\n", g_fs.block_bitmap_size);
    g_fs.block_bitmap = (uint32_t *)g_block_bitmap_storage;

    print("[FS] Initializing inode bitmap ({d} bytes)...\n", g_fs.inode_bitmap_size);
    g_fs.inode_bitmap = (uint32_t *)g_inode_bitmap_storage;

    print("[FS] Initializing inode table ({d} bytes)...\n", 
          (uint32_t)(sizeof(inode_t) * g_fs.total_inodes));
    g_fs.inode_table = g_inode_table_storage;

    /* Zero out the bitmaps and inode table */
    print("[FS] Zeroing bitmaps...\n");
    for (uint32_t i = 0; i < g_fs.block_bitmap_size; i++) {
        ((uint8_t*)g_fs.block_bitmap)[i] = 0;
    }
    for (uint32_t i = 0; i < g_fs.inode_bitmap_size; i++) {
        ((uint8_t*)g_fs.inode_bitmap)[i] = 0;
    }
    
    print("[FS] Zeroing inode table...\n");
    for (uint32_t i = 0; i < g_fs.total_inodes; i++) {
        memset(&g_fs.inode_table[i], 0, sizeof(inode_t));
    }

    print("[FS] Initializing file handle table...\n");
    /* Initialize file handle table */
    for (int i = 0; i < MAX_FILES_OPEN; i++) {
        g_file_table[i].is_open = false;
        g_file_table[i].inode_number = 0;
        g_file_table[i].offset = 0;
        g_file_table[i].flags = 0;
    }

    print("[FS] Creating root directory...\n");
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
        print("[FS] Root inode created at address: {x}\n", (uint32_t)g_fs.root_inode);
        print("[FS] Total blocks: {d}, Total inodes: {d}\n", 
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
    inode->link_count = 1;
    inode->perms = perms;

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

        if (byte_idx >= g_fs.block_bitmap_size) {
            print("[FS] ERROR: Bitmap index out of bounds\n");
            return 0xFFFFFFFF;
        }

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
        print("[FS] ERROR: Invalid block number: {d}\n", block_number);
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
int32_t fs_open(const char *filename __attribute__((unused)), uint8_t flags)
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
size_t fs_write(int32_t handle, const void *buffer __attribute__((unused)), size_t bytes __attribute__((unused)))
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !g_file_table[handle].is_open) {
        print("[FS] ERROR: Invalid file handle for write\n");
        return 0;
    }

    /* Placeholder implementation */
    return 0;
}

/**
 * Add an entry to a directory
 */
int32_t fs_add_dir_entry(inode_t *dir_inode, const char *name, uint32_t inode_num)
{
    if (!dir_inode || dir_inode->type != FILE_TYPE_DIRECTORY) {
        print("[FS] ERROR: Not a directory\n");
        return -1;
    }

    if (dir_inode->entry_count >= MAX_DIR_ENTRIES) {
        print("[FS] ERROR: Directory is full\n");
        return -1;
    }

    /* Check if name already exists */
    for (uint32_t i = 0; i < dir_inode->entry_count; i++) {
        if (fs_strcmp(dir_inode->entries[i].filename, name) == 0) {
            print("[FS] ERROR: Entry already exists\n");
            return -1;
        }
    }

    /* Add new entry */
    uint32_t idx = dir_inode->entry_count;
    dir_inode->entries[idx].inode_number = inode_num;
    
    /* Copy filename */
    uint32_t name_len = 0;
    while (name[name_len] && name_len < (MAX_FILENAME_LEN - 1)) {
        dir_inode->entries[idx].filename[name_len] = name[name_len];
        name_len++;
    }
    dir_inode->entries[idx].filename[name_len] = '\0';
    
    dir_inode->entry_count++;
    return 0;
}

/**
 * Find an entry in a directory by name
 */
inode_t* fs_find_in_dir(inode_t *dir_inode, const char *name)
{
    if (!dir_inode || dir_inode->type != FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    for (uint32_t i = 0; i < dir_inode->entry_count; i++) {
        if (fs_strcmp(dir_inode->entries[i].filename, name) == 0) {
            return fs_inode_get(dir_inode->entries[i].inode_number);
        }
    }

    return NULL;
}

/**
 * Create a directory in a parent directory
 */
int32_t fs_mkdir(const char *name, inode_t *parent, file_perms_t perms)
{
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        print("[FS] ERROR: Parent is not a directory\n");
        return -1;
    }

    /* Create new directory inode */
    inode_t *dir_inode = fs_inode_create(FILE_TYPE_DIRECTORY, perms);
    if (!dir_inode) {
        print("[FS] ERROR: Failed to create directory inode\n");
        return -1;
    }

    /* Add entry to parent */
    if (fs_add_dir_entry(parent, name, dir_inode->inode_number) != 0) {
        print("[FS] ERROR: Failed to add directory entry to parent\n");
        fs_inode_delete(dir_inode->inode_number);
        return -1;
    }

    print("[FS] Created directory '{s}' (inode {d})\n", name, dir_inode->inode_number);
    return dir_inode->inode_number;
}

/**
 * Remove a directory from parent
 */
int32_t fs_rmdir(const char *name, inode_t *parent)
{
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        print("[FS] ERROR: Parent is not a directory\n");
        return -1;
    }

    /* Find the entry */
    uint32_t idx = 0xFFFFFFFF;
    for (uint32_t i = 0; i < parent->entry_count; i++) {
        if (fs_strcmp(parent->entries[i].filename, name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == 0xFFFFFFFF) {
        print("[FS] ERROR: Entry not found\n");
        return -1;
    }

    uint32_t inode_num = parent->entries[idx].inode_number;
    inode_t *inode = fs_inode_get(inode_num);

    /* Check if directory is empty (except . and ..) */
    if (inode && inode->type == FILE_TYPE_DIRECTORY && inode->entry_count > 0) {
        print("[FS] ERROR: Directory not empty\n");
        return -1;
    }

    /* Remove entry by shifting */
    for (uint32_t i = idx; i < parent->entry_count - 1; i++) {
        parent->entries[i] = parent->entries[i + 1];
    }
    parent->entry_count--;

    /* Delete the inode */
    if (inode) {
        fs_inode_delete(inode_num);
    }

    print("[FS] Removed directory '{s}'\n", name);
    return 0;
}

/**
 * Read directory entries
 */
dir_entry_t* fs_readdir(inode_t *dir_inode, uint32_t *count)
{
    if (!dir_inode || dir_inode->type != FILE_TYPE_DIRECTORY) {
        return NULL;
    }

    if (count) {
        *count = dir_inode->entry_count;
    }

    return dir_inode->entries;
}

/**
 * String comparison helper
 */
int32_t fs_strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/**
 * Seek in a file
 */
int32_t fs_seek(int32_t handle, int32_t offset __attribute__((unused)), int whence __attribute__((unused)))
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !g_file_table[handle].is_open) {
        print("[FS] ERROR: Invalid file handle for seek\n");
        return -1;
    }

    /* Placeholder implementation */
    return 0;
}

/**
 * Parse a path string into components
 * e.g., "/home/user/file.txt" -> ["home", "user", "file.txt"]
 */
path_t* fs_parse_path(const char *path)
{
    static path_t parsed_path;
    parsed_path.depth = 0;
    parsed_path.is_absolute = false;

    if (!path || path[0] == '\0') {
        return &parsed_path;
    }

    /* Check if absolute path */
    uint32_t start = 0;
    if (path[0] == '/') {
        parsed_path.is_absolute = true;
        start = 1;
    }

    /* Parse path components */
    uint32_t i = start;
    uint32_t component_len = 0;
    char current_component[MAX_FILENAME_LEN];

    while (path[i] && parsed_path.depth < MAX_PATH_DEPTH) {
        if (path[i] == '/') {
            if (component_len > 0) {
                current_component[component_len] = '\0';
                /* Copy component to path structure */
                uint32_t j = 0;
                while (j < component_len && j < (MAX_FILENAME_LEN - 1)) {
                    parsed_path.components[parsed_path.depth][j] = current_component[j];
                    j++;
                }
                parsed_path.components[parsed_path.depth][j] = '\0';
                parsed_path.depth++;
                component_len = 0;
            }
        } else {
            if (component_len < (MAX_FILENAME_LEN - 1)) {
                current_component[component_len++] = path[i];
            }
        }
        i++;
    }

    /* Add final component */
    if (component_len > 0 && parsed_path.depth < MAX_PATH_DEPTH) {
        current_component[component_len] = '\0';
        uint32_t j = 0;
        while (j < component_len && j < (MAX_FILENAME_LEN - 1)) {
            parsed_path.components[parsed_path.depth][j] = current_component[j];
            j++;
        }
        parsed_path.components[parsed_path.depth][j] = '\0';
        parsed_path.depth++;
    }

    return &parsed_path;
}

/**
 * Traverse a path and return the inode it points to
 */
inode_t* fs_traverse_path(const char *path)
{
    path_t *parsed = fs_parse_path(path);

    if (parsed->depth == 0) {
        return g_fs.root_inode;
    }

    /* Start from root or current directory */
    inode_t *current = g_fs.root_inode;

    /* Follow each path component */
    for (uint32_t i = 0; i < parsed->depth; i++) {
        if (!current || current->type != FILE_TYPE_DIRECTORY) {
            return NULL;
        }

        current = fs_find_in_dir(current, parsed->components[i]);
        if (!current) {
            return NULL;
        }
    }

    return current;
}

/**
 * Get parent directory and filename from a path
 * e.g., "/home/user/file.txt" -> ("/home/user", "file.txt")
 */
inode_t* fs_get_parent_dir(const char *path, char *out_filename)
{
    path_t *parsed = fs_parse_path(path);

    if (parsed->depth == 0) {
        return NULL;
    }

    /* Copy the last component as filename */
    if (out_filename && parsed->depth > 0) {
        uint32_t i = 0;
        while (i < MAX_FILENAME_LEN - 1 && parsed->components[parsed->depth - 1][i]) {
            out_filename[i] = parsed->components[parsed->depth - 1][i];
            i++;
        }
        out_filename[i] = '\0';
    }

    /* If only one component, parent is root */
    if (parsed->depth == 1) {
        return g_fs.root_inode;
    }

    /* Navigate to parent directory */
    inode_t *parent = g_fs.root_inode;
    for (uint32_t i = 0; i < parsed->depth - 1; i++) {
        parent = fs_find_in_dir(parent, parsed->components[i]);
        if (!parent) {
            return NULL;
        }
    }

    return parent;
}

/**
 * Create a regular file at a given path
 */
int32_t fs_create_file(const char *path, file_perms_t perms)
{
    char filename[MAX_FILENAME_LEN];
    inode_t *parent = fs_get_parent_dir(path, filename);

    if (!parent) {
        print("[FS] ERROR: Parent directory not found\n");
        return -1;
    }

    /* Create new file inode */
    inode_t *file_inode = fs_inode_create(FILE_TYPE_REGULAR, perms);
    if (!file_inode) {
        print("[FS] ERROR: Failed to create file inode\n");
        return -1;
    }

    /* Add entry to parent */
    if (fs_add_dir_entry(parent, filename, file_inode->inode_number) != 0) {
        print("[FS] ERROR: Failed to add file entry to parent\n");
        fs_inode_delete(file_inode->inode_number);
        return -1;
    }

    print("[FS] Created file '{s}' (inode {d})\n", filename, file_inode->inode_number);
    return file_inode->inode_number;
}

/**
 * Create a directory at a given path
 */
int32_t fs_create_dir(const char *path, file_perms_t perms)
{
    char dirname[MAX_FILENAME_LEN];
    inode_t *parent = fs_get_parent_dir(path, dirname);

    if (!parent) {
        print("[FS] ERROR: Parent directory not found\n");
        return -1;
    }

    return fs_mkdir(dirname, parent, perms);
}

/**
 * Remove a file at a given path
 */
int32_t fs_remove_file(const char *path)
{
    char filename[MAX_FILENAME_LEN];
    inode_t *parent = fs_get_parent_dir(path, filename);

    if (!parent) {
        print("[FS] ERROR: Parent directory not found\n");
        return -1;
    }

    /* Find and remove the entry */
    for (uint32_t i = 0; i < parent->entry_count; i++) {
        if (fs_strcmp(parent->entries[i].filename, filename) == 0) {
            uint32_t inode_num = parent->entries[i].inode_number;
            
            /* Remove entry by shifting */
            for (uint32_t j = i; j < parent->entry_count - 1; j++) {
                parent->entries[j] = parent->entries[j + 1];
            }
            parent->entry_count--;

            /* Delete the inode */
            fs_inode_delete(inode_num);
            print("[FS] Removed file '{s}'\n", filename);
            return 0;
        }
    }

    print("[FS] ERROR: File not found\n");
    return -1;
}

/**
 * Remove a directory at a given path
 */
int32_t fs_remove_dir(const char *path)
{
    char dirname[MAX_FILENAME_LEN];
    inode_t *parent = fs_get_parent_dir(path, dirname);

    if (!parent) {
        print("[FS] ERROR: Parent directory not found\n");
        return -1;
    }

    return fs_rmdir(dirname, parent);
}

/**
 * Find a file/directory by path
 */
inode_t* fs_find(const char *path)
{
    return fs_traverse_path(path);
}
