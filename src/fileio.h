#include "main.h"

typedef struct directory_entry
{
    char file_name[255 + 1];
    uint64_t file_size;
} fs_directory_entry_t;

// Create a software driver for the file system (FAT, NTFS, etc)
typedef struct fs_file_driver
{
    int (*init)(const char drive_letter);
    int (*open)(const char *path, int flags);
    ssize_t (*read)(int fd, void *buffer, size_t count);
    ssize_t (*write)(int pdrv, const void *buffer, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    int (*close)(int fd);

    void *(*opendir)(const char *path);
    fs_directory_entry_t *(*readdir)(void *handle, fs_directory_entry_t *entry);
    void (*closedir)(void *handle);
} fs_sw_driver_t;

// Create a hardware driver for the file system (USB, ATA, etc)
typedef struct filesystem_io_driver
{
    int8_t (*disk_status)(char drive_letter);
    int8_t (*disk_initialize)(char drive_letter);
    int8_t (*disk_read)(char drive_letter, void *buffer, uint64_t sector, uint32_t count);
    int8_t (*disk_write)(char drive_letter, const void *buffer, uint64_t sector, uint32_t count);
    int8_t (*disk_ioctl)(char drive_letter, uint8_t cmd, void *buff);
} fs_hw_driver_t;

typedef struct file_system_driver
{
    char drive_letter;
    fs_sw_driver_t *sw;
    fs_hw_driver_t *hw;
    struct file_system_driver *next;
} file_system_driver_t;

typedef struct file_pointer
{
    int handle;
    void *user_data;
    fs_sw_driver_t *sw;
    fs_hw_driver_t *hw;
} file_pointer_t;

typedef struct directory {
    fs_sw_driver_t *sw;
    fs_hw_driver_t *hw;
    fs_directory_entry_t entry;
    void *user_data;
} directory_t;

file_system_driver_t *fileio_find_driver(const char drive_letter);
int8_t fileio_register_driver(const char drive_letter, fs_hw_driver_t *hw, fs_sw_driver_t *sw);
int8_t fileio_unregister_driver(const char drive_letter);

directory_t *opendir(const char *path);
fs_directory_entry_t *readdir(directory_t *dir);
void closedir(directory_t *dir);