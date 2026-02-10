#include "fs_api.h"
#include "filesystem.h"
#include "print.h"

/**
 * File Operations
 */

int32_t fs_api_open(const char *path, uint8_t flags)
{
    if (!path) {
        return FS_ERR_INVALID;
    }

    inode_t *inode = fs_find(path);

    if (!inode) {
        /* If create flag is set, create the file */
        if (flags & FS_MODE_CREATE) {
            file_perms_t perms = { .permissions = 0644, .uid = 0, .gid = 0 };
            int32_t result = fs_create_file(path, perms);
            if (result < 0) {
                return FS_ERR_FULL;
            }
            inode = fs_find(path);
            if (!inode) {
                return FS_ERR_INVALID;
            }
        } else {
            return FS_ERR_NOT_FOUND;
        }
    }

    /* Only regular files can be opened */
    if (inode->type != FILE_TYPE_REGULAR) {
        return FS_ERR_NOT_DIR;
    }

    /* Get a file handle */
    int32_t handle = fs_open(path, flags);
    if (handle < 0) {
        return FS_ERR_FULL;
    }

    return handle;
}

int32_t fs_api_close(int32_t handle)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN) {
        return FS_ERR_INVALID;
    }

    fs_close(handle);
    return FS_OK;
}

int32_t fs_api_read(int32_t handle, void *buffer, uint32_t size)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !buffer) {
        return FS_ERR_INVALID;
    }

    /* TODO: Implement actual file reading */
    return FS_OK;
}

int32_t fs_api_write(int32_t handle, const void *buffer, uint32_t size)
{
    if (handle < 0 || handle >= MAX_FILES_OPEN || !buffer) {
        return FS_ERR_INVALID;
    }

    /* TODO: Implement actual file writing */
    return FS_OK;
}

/**
 * Directory Operations
 */

int32_t fs_api_mkdir(const char *path)
{
    if (!path || path[0] == '\0' || path[0] == ' ') {
        return FS_ERR_INVALID;
    }

    /* Check if already exists */
    if (fs_find(path) != NULL) {
        return FS_ERR_EXISTS;
    }

    file_perms_t perms = { .permissions = 0755, .uid = 0, .gid = 0 };
    int32_t result = fs_create_dir(path, perms);

    if (result < 0) {
        return FS_ERR_FULL;
    }

    return FS_OK;
}

int32_t fs_api_rmdir(const char *path)
{
    if (!path) {
        return FS_ERR_INVALID;
    }

    inode_t *inode = fs_find(path);
    if (!inode) {
        return FS_ERR_NOT_FOUND;
    }

    if (inode->type != FILE_TYPE_DIRECTORY) {
        return FS_ERR_NOT_DIR;
    }

    if (inode->entry_count > 0) {
        return FS_ERR_NOT_EMPTY;
    }

    int32_t result = fs_remove_dir(path);
    if (result < 0) {
        return FS_ERR_INVALID;
    }

    return FS_OK;
}

int32_t fs_api_remove(const char *path)
{
    if (!path) {
        return FS_ERR_INVALID;
    }

    inode_t *inode = fs_find(path);
    if (!inode) {
        return FS_ERR_NOT_FOUND;
    }

    if (inode->type == FILE_TYPE_DIRECTORY) {
        return FS_ERR_NOT_DIR;
    }

    int32_t result = fs_remove_file(path);
    if (result < 0) {
        return FS_ERR_INVALID;
    }

    return FS_OK;
}

int32_t fs_api_listdir(const char *path, fs_dirent_t *entries, uint32_t max_entries)
{
    if (!path || !entries || max_entries == 0) {
        return FS_ERR_INVALID;
    }

    inode_t *dir = fs_find(path);
    if (!dir) {
        return FS_ERR_NOT_FOUND;
    }

    if (dir->type != FILE_TYPE_DIRECTORY) {
        return FS_ERR_NOT_DIR;
    }

    uint32_t count = 0;
    dir_entry_t *dir_entries = fs_readdir(dir, &count);

    if (count > max_entries) {
        count = max_entries;
    }

    /* Convert directory entries to API format */
    for (uint32_t i = 0; i < count; i++) {
        inode_t *entry_inode = fs_inode_get(dir_entries[i].inode_number);
        
        /* Copy name */
        uint32_t j = 0;
        while (j < 255 && dir_entries[i].filename[j]) {
            entries[i].name[j] = dir_entries[i].filename[j];
            j++;
        }
        entries[i].name[j] = '\0';

        /* Copy metadata */
        if (entry_inode) {
            entries[i].type = entry_inode->type;
            entries[i].size = entry_inode->size;
        } else {
            entries[i].type = 0;
            entries[i].size = 0;
        }
    }

    return (int32_t)count;
}

/**
 * File Inspection Operations
 */

int32_t fs_api_stat(const char *path, fs_stat_t *stat)
{
    if (!path || !stat) {
        return FS_ERR_INVALID;
    }

    inode_t *inode = fs_find(path);
    if (!inode) {
        return FS_ERR_NOT_FOUND;
    }

    stat->inode_number = inode->inode_number;
    stat->type = inode->type;
    stat->size = inode->size;
    stat->created_time = inode->created_time;
    stat->modified_time = inode->modified_time;

    return FS_OK;
}

bool fs_api_exists(const char *path)
{
    if (!path) {
        return false;
    }

    return fs_find(path) != NULL;
}

bool fs_api_is_directory(const char *path)
{
    if (!path) {
        return false;
    }

    inode_t *inode = fs_find(path);
    return inode && inode->type == FILE_TYPE_DIRECTORY;
}

bool fs_api_is_file(const char *path)
{
    if (!path) {
        return false;
    }

    inode_t *inode = fs_find(path);
    return inode && inode->type == FILE_TYPE_REGULAR;
}


/**
 * Current Working Directory Operations
 */

const char* fs_api_getcwd(void)
{
    return g_fs.cwd_path;
}

int32_t fs_api_chdir(const char *path)
{
    if (!path || path[0] == '\0') {
        return FS_ERR_INVALID;
    }

    // Find the target directory
    inode_t *target = fs_traverse_path(path);
    if (!target) {
        return FS_ERR_NOT_FOUND;
    }

    // Ensure it's a directory
    if (target->type != FILE_TYPE_DIRECTORY) {
        return FS_ERR_NOT_DIR;
    }

    // Update current directory pointer
    g_fs.current_dir = target;

    // Update cwd_path string
    // If path is absolute, use it directly; otherwise, append to current path
    if (path[0] == '/') {
        // Absolute path: copy directly
        uint32_t i = 0;
        while (i < (MAX_PATH_DEPTH * MAX_FILENAME_LEN - 1) && path[i]) {
            g_fs.cwd_path[i] = path[i];
            i++;
        }
        g_fs.cwd_path[i] = '\0';
    } else {
        // Relative path: construct full path
        char temp_path[MAX_PATH_DEPTH * MAX_FILENAME_LEN] = {0};
        uint32_t len = 0;

        // Copy current path
        while (len < (MAX_PATH_DEPTH * MAX_FILENAME_LEN - 1) && g_fs.cwd_path[len]) {
            temp_path[len] = g_fs.cwd_path[len];
            len++;
        }

        // Append separator if not root
        if (len > 1 && temp_path[len - 1] != '/') {
            temp_path[len++] = '/';
        } else if (len == 1 && temp_path[0] == '/') {
            len = 1;
        }

        // Append the relative path
        uint32_t i = 0;
        while (i < (MAX_PATH_DEPTH * MAX_FILENAME_LEN - len - 1) && path[i]) {
            temp_path[len + i] = path[i];
            i++;
        }
        temp_path[len + i] = '\0';

        // Copy back to cwd_path
        i = 0;
        while (i < (MAX_PATH_DEPTH * MAX_FILENAME_LEN - 1) && temp_path[i]) {
            g_fs.cwd_path[i] = temp_path[i];
            i++;
        }
        g_fs.cwd_path[i] = '\0';
    }

    return FS_OK;
}

/**
 * Utility Operations
 */

const char* fs_api_strerror(int32_t error_code)
{
    switch (error_code) {
        case FS_OK:
            return "No error";
        case FS_ERR_NOT_FOUND:
            return "File or directory not found";
        case FS_ERR_INVALID:
            return "Invalid argument";
        case FS_ERR_FULL:
            return "Filesystem full";
        case FS_ERR_PERMISSION:
            return "Permission denied";
        case FS_ERR_EXISTS:
            return "File or directory already exists";
        case FS_ERR_NOT_DIR:
            return "Not a directory";
        case FS_ERR_NOT_EMPTY:
            return "Directory not empty";
        default:
            return "Unknown error";
    }
}
