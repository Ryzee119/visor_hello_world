#include "main.h"

struct directory_entry;

enum
{
    FS_IO_SYNC,
    FS_IO_GET_SECTOR_COUNT,
    FS_IO_GET_SECTOR_SIZE,
    FS_IO_GET_BLOCK_SIZE
};
typedef uint8_t fs_ioctrl_cmd_t;

typedef struct fs_io
{
    int (*init)(const char drive_letter);
    int (*open)(const char *path, int flags);
    ssize_t (*read)(int fd, void *buffer, size_t count);
    ssize_t (*write)(int pdrv, const void *buffer, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    int (*close)(int fd);

    // Optional, set to NULL if not supported
    void *(*opendir)(const char *path);
    struct directory_entry *(*readdir)(void *handle, struct directory_entry *entry);
    void (*closedir)(void *handle);
} fs_io_t;

typedef struct fs_io_ll
{
    ssize_t (*read)(void *handle, void *buffer, uint64_t offset, size_t count);
    ssize_t (*write)(void *handle, const void *buffer, uint64_t offset, size_t count);
    int8_t (*ioctrl)(const char drive_letter, fs_ioctrl_cmd_t cmd, void *buff);
} fs_io_ll_t;

typedef struct file_io_handle
{
    char drive_letter;
    struct file_io_handle *next;
    fs_io_t *io;
    fs_io_ll_t *io_ll;
    void *user_data;
} file_io_handle_t;

typedef struct file_handle
{
    uint32_t handle;
    void *user_data;
} file_handle_t;

typedef struct directory_entry
{
    char file_name[255 + 1];
    uint64_t file_size;
} directory_entry_t;

typedef struct directory_handle
{
    directory_entry_t entry;
    void *user_data;
} directory_handle_t;

// Dirent-like functions
directory_handle_t *opendir(const char *path);
directory_entry_t *readdir(directory_handle_t *dir);
void closedir(directory_handle_t *dir);

#define DRIVE_ACCESS_RAW(driver_letter) (0x80 | driver_letter)

file_io_handle_t *fileio_find_io_handle(const char drive_letter);
int8_t fileio_register_io_handle(const char drive_letter, fs_io_t *io, fs_io_ll_t *io_ll);
int8_t fileio_unregister_io_handle(const char drive_letter);
