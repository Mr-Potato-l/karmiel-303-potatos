#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* File System Constants */
#define MAX_FILENAME_LEN    256
#define MAX_FILES_OPEN      16
#define MAX_FILES_TOTAL     512
#define INODE_SIZE          256
#define BLOCK_SIZE          4096

/* File Types */
typedef enum {
    FILE_TYPE_REGULAR = 0,
    FILE_TYPE_DIRECTORY = 1,
    FILE_TYPE_DEVICE = 2
} file_type_t;

/* File Permissions (Unix-style) */
typedef struct {
    uint16_t permissions;  /* rwxrwxrwx */
    uint16_t uid;
    uint16_t gid;
} file_perms_t;

/* Inode structure - represents a file/directory */
typedef struct {
    uint32_t inode_number;
    file_type_t type;
    size_t size;
    uint32_t block_count;
    uint32_t *blocks;              /* Pointers to data blocks */
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint16_t link_count;
    file_perms_t perms;
} inode_t;

/* Directory entry */
typedef struct {
    uint32_t inode_number;
    char filename[MAX_FILENAME_LEN];
} dir_entry_t;

/* File handle for open files */
typedef struct {
    uint32_t inode_number;
    size_t offset;                 /* Current read/write position */
    bool is_open;
    uint8_t flags;                 /* Read, write, append flags */
} file_handle_t;

/* File System Super Block - metadata about the filesystem */
typedef struct {
    uint32_t magic_number;         /* Identifies filesystem type */
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_bitmap_size;
    uint32_t inode_bitmap_size;
    uint32_t *block_bitmap;        /* Track free/used blocks */
    uint32_t *inode_bitmap;        /* Track free/used inodes */
    inode_t *inode_table;          /* Array of all inodes */
    inode_t *root_inode;           /* Pointer to root directory */
} filesystem_t;

/* File System Operations */
void fs_init(void);
void fs_shutdown(void);

/* Inode operations */
inode_t* fs_inode_create(file_type_t type, file_perms_t perms);
void fs_inode_delete(uint32_t inode_number);
inode_t* fs_inode_get(uint32_t inode_number);

/* File operations */
int32_t fs_open(const char *filename, uint8_t flags);
void fs_close(int32_t handle);
size_t fs_read(int32_t handle, void *buffer, size_t bytes);
size_t fs_write(int32_t handle, const void *buffer, size_t bytes);
int32_t fs_seek(int32_t handle, int32_t offset, int whence);

/* Directory operations */
int32_t fs_mkdir(const char *path, file_perms_t perms);
int32_t fs_rmdir(const char *path);
dir_entry_t* fs_readdir(inode_t *dir_inode);

/* Block operations */
uint32_t fs_allocate_block(void);
void fs_free_block(uint32_t block_number);
void* fs_get_block_address(uint32_t block_number);

/* Helper functions */
uint32_t fs_get_free_inode(void);
void fs_set_inode_bitmap(uint32_t inode_num, bool used);
void fs_set_block_bitmap(uint32_t block_num, bool used);

#endif
