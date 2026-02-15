#ifndef FS_API_H
#define FS_API_H

#include <stdint.h>
#include <stdbool.h>

/* File API error codes */
#define FS_OK               0
#define FS_ERR_NOT_FOUND    -1
#define FS_ERR_INVALID      -2
#define FS_ERR_FULL         -3
#define FS_ERR_PERMISSION   -4
#define FS_ERR_EXISTS       -5
#define FS_ERR_NOT_DIR      -6
#define FS_ERR_NOT_EMPTY    -7

/* File open flags */
#define FS_MODE_READ        0x01
#define FS_MODE_WRITE       0x02
#define FS_MODE_APPEND      0x04
#define FS_MODE_CREATE      0x08

#define MAX_DIR_ENTRIES     8

/* File info structure */
typedef struct {
    uint32_t inode_number;
    uint8_t type;           /* 0=file, 1=directory, 2=device */
    uint32_t size;
    uint32_t created_time;
    uint32_t modified_time;
} fs_stat_t;

/* Directory entry for listing */
typedef struct {
    char name[256];
    uint8_t type;           /* 0=file, 1=directory, 2=device */
    uint32_t size;
} fs_dirent_t;

/**
 * File Operations
 */

/**
 * Open a file and return a file handle
 * @param path      Path to file (e.g., "/home/user/file.txt")
 * @param flags     Open flags (FS_MODE_READ, FS_MODE_WRITE, etc.)
 * @return          File handle (>= 0) on success, error code on failure
 */
int32_t fs_api_open(const char *path, uint8_t flags);

/**
 * Close a file handle
 * @param handle    File handle
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_close(int32_t handle);

/**
 * Read from a file
 * @param handle    File handle
 * @param buffer    Buffer to read into
 * @param size      Number of bytes to read
 * @return          Number of bytes read, or error code
 */
int32_t fs_api_read(int32_t handle, void *buffer, uint32_t size);

/**
 * Write to a file
 * @param handle    File handle
 * @param buffer    Buffer to write from
 * @param size      Number of bytes to write
 * @return          Number of bytes written, or error code
 */
int32_t fs_api_write(int32_t handle, const void *buffer, uint32_t size);

/**
 * Directory Operations
 */

/**
 * Create a directory
 * @param path      Path to directory
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_mkdir(const char *path);

/**
 * Remove a directory (must be empty)
 * @param path      Path to directory
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_rmdir(const char *path);

/**
 * Remove a file
 * @param path      Path to file
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_remove(const char *path);

/**
 * List directory contents
 * @param path      Path to directory
 * @param entries   Buffer to store entries (must have space for at least 8 entries)
 * @param max_entries Maximum number of entries to return
 * @return          Number of entries listed, or error code
 */
int32_t fs_api_listdir(const char *path, fs_dirent_t *entries, uint32_t max_entries);

/**
 * File Inspection Operations
 */

/**
 * Get file/directory information
 * @param path      Path to file or directory
 * @param stat      Structure to fill with file info
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_stat(const char *path, fs_stat_t *stat);

/**
 * Check if a file or directory exists
 * @param path      Path to check
 * @return          true if exists, false otherwise
 */
bool fs_api_exists(const char *path);

/**
 * Check if path is a directory
 * @param path      Path to check
 * @return          true if directory, false otherwise
 */
bool fs_api_is_directory(const char *path);

/**
 * Check if path is a regular file
 * @param path      Path to check
 * @return          true if file, false otherwise
 */
bool fs_api_is_file(const char *path);

int32_t fs_api_chdir(const char *path);


/**
 * Utility Operations
 */

/**
 * Get human-readable error message
 * @param error_code Error code
 * @return          Error message string
 */
const char* fs_api_strerror(int32_t error_code);

/**
 * Current Working Directory Operations
 */

/**
 * Get the current working directory path
 * @return          Pointer to current working directory path string
 */
const char* fs_api_getcwd(void);

/**
 * Change the current working directory
 * @param path      Path to directory (relative or absolute)
 * @return          FS_OK on success, error code on failure
 */
int32_t fs_api_chdir(const char *path);

#endif
