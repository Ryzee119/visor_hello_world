#include "main.h"

#ifndef FILEIO_H
#define FILEIO_H

struct directory_entry;
struct file_handle;
struct file_io_driver;
 
enum
{
    FS_IO_SYNC,
    FS_IO_GET_SECTOR_COUNT,
    FS_IO_GET_SECTOR_SIZE,
    FS_IO_GET_BLOCK_SIZE
};
typedef uint8_t fs_ioctrl_cmd_t;
typedef void user_file_handle_t;
typedef void user_dir_handle_t;
typedef void fs_user_ll_handle_t;

typedef struct fs_io 
{
    int (*init)(struct file_io_driver *driver, void **user_data, void *arg);
    void (*deinit)(char drive_letter, void *user_data);
    user_file_handle_t *(*open)(const char *path, int flags);
    ssize_t (*read)(user_file_handle_t *fp, void *buffer, size_t count);
    ssize_t (*write)(user_file_handle_t *fp, const void *buffer, size_t count);
    off_t (*lseek)(user_file_handle_t *fp, off_t offset, int whence);
    int (*close)(user_file_handle_t *fp);

    user_dir_handle_t *(*opendir)(const char *path);
    struct directory_entry *(*readdir)(user_dir_handle_t *dir, struct directory_entry *entry);
    void (*closedir)(user_dir_handle_t *dir);
} fs_io_t;

typedef struct fs_io_ll
{
    int8_t (*init)(char drive_letter, fs_user_ll_handle_t **handle, void *arg);
    void (*deinit)(char drive_letter, fs_user_ll_handle_t *handle);
    ssize_t (*read)(fs_user_ll_handle_t *handle, void *buffer, uint64_t sector_offset, size_t sector_count);
    ssize_t (*write)(fs_user_ll_handle_t *handle, const void *buffer, uint64_t sector_offset, size_t sector_count);
    int8_t (*ioctrl)(fs_user_ll_handle_t *handle, fs_ioctrl_cmd_t cmd, void *buff);
} fs_io_ll_t;

typedef struct file_io_driver
{
    char drive_letter;
    fs_io_t *io;
    fs_io_ll_t *io_ll;
    fs_user_ll_handle_t *ll_handle;
    SemaphoreHandle_t mutex;
    void *user_data;
    struct file_io_driver *next;
} file_io_driver_t;

typedef struct file_handle
{
    file_io_driver_t *driver;
    user_file_handle_t *user_handle;
} file_handle_t;

typedef struct directory_entry
{
    char file_name[256];
    uint64_t file_size;
} directory_entry_t;

typedef struct directory_handle
{
    file_io_driver_t *driver;
    directory_entry_t entry;
    user_file_handle_t *user_handle;
} directory_handle_t;

// Dirent-like functions
directory_handle_t *opendir(const char *path);
directory_entry_t *readdir(directory_handle_t *dir);
void closedir(directory_handle_t *dir);

file_io_driver_t *fileio_find_driver(const char drive_letter);
int8_t fileio_register_driver(const char drive_letter, fs_io_t *io, fs_io_ll_t *io_ll, void *arg, void *ll_arg);
int8_t fileio_unregister_driver(const char drive_letter);

#endif