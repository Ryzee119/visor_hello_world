#include "main.h"
#include <fcntl.h>
#include <unistd.h>

int open(const char *path, int flags, ...)
{
    char drive_letter = path[0];

    // Need to register a driver against the drive letter first
    file_system_driver_t *fs_driver = fileio_find_driver(drive_letter);
    if (fs_driver == NULL) {
        return -1;
    }

    // Create a copy link to this file handle
    file_pointer_t *fp = pvPortMalloc(sizeof(file_pointer_t));
    if (fp == NULL) {
        return -1;
    }

    fp->hw = fs_driver->hw;
    fp->sw = fs_driver->sw;
    fp->handle = fp->sw->open(path, flags);

    if (fp->handle < 0) {
        vPortFree(fp);
        return -1;
    }

    return (int)fp;
}

int close(int fd)
{
    file_pointer_t *fp = (file_pointer_t *)fd;
    fp->sw->close(fp->handle);
    vPortFree(fp);
    return 0;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    file_pointer_t *fp = (file_pointer_t *)fd;
    return fp->sw->read(fp->handle, buffer, count);
}

ssize_t write(int fd, const void *buffer, size_t count)
{
    file_pointer_t *fp = (file_pointer_t *)fd;
    return fp->sw->write(fp->handle, buffer, count);
}

off_t lseek(int fd, off_t offset, int whence)
{
    file_pointer_t *fp = (file_pointer_t *)fd;
    return fp->sw->lseek(fp->handle, offset, whence);
}


// No dirent in picolibc, so we need to define our own dirent-like functions
directory_t *opendir(const char *path)
{
    char drive_letter = path[0];
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return NULL;
    }

    directory_t *dir = pvPortMalloc(sizeof(directory_t));
    if (dir == NULL) {
        return NULL;
    }

    dir->sw = driver->sw;
    dir->hw = driver->hw;
    dir->entry.file_name[0] = 0;
    dir->entry.file_size = 0;
    dir->user_data = dir->sw->opendir(path);
    if (dir->user_data == NULL) {
        vPortFree(dir);
        return NULL;
    }

    return dir;
}

fs_directory_entry_t *readdir(directory_t *dir)
{
    return dir->sw->readdir(dir->user_data, &dir->entry);
}

void closedir(directory_t *dir)
{
    dir->sw->closedir(dir->user_data);
    vPortFree(dir);
}

static file_system_driver_t *fs_driver_head = NULL;

file_system_driver_t *fileio_find_driver(const char drive_letter)
{
    taskENTER_CRITICAL();
    file_system_driver_t *driver = fs_driver_head;
    while (driver != NULL) {
        if (driver->drive_letter == drive_letter) {
            taskEXIT_CRITICAL();
            return driver;
        }
        driver = driver->next;
    }
    taskEXIT_CRITICAL();
    printf("Driver not found for drive \'%c\'\n", drive_letter);
    return NULL;
}

int8_t fileio_register_driver(const char drive_letter, fs_hw_driver_t *hw, fs_sw_driver_t *sw)
{
    file_system_driver_t *driver = pvPortMalloc(sizeof(file_system_driver_t));
    if (driver == NULL) {
        return -1;
    }

    driver->drive_letter = drive_letter;
    driver->hw = hw;
    driver->sw = sw;

    taskENTER_CRITICAL();

    if (fs_driver_head == NULL) {
        fs_driver_head = driver;
        driver->next = NULL;
    } else {
        file_system_driver_t *p = fs_driver_head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = driver;
        driver->next = NULL;
    }

    taskEXIT_CRITICAL();

    if (hw->disk_initialize(drive_letter) != 0) {
        return -1;
    }
    
    if (sw->init(drive_letter) != 0) {
        return -1;
    }

    return 0;
}

int8_t fileio_unregister_driver(const char drive_letter)
{
    taskENTER_CRITICAL();

    file_system_driver_t *driver = fs_driver_head;
    file_system_driver_t *prev = NULL;
    while (driver != NULL) {
        if (driver->drive_letter == drive_letter) {
            if (prev == NULL) {
                fs_driver_head = driver->next;
            } else {
                prev->next = driver->next;
            }
            taskEXIT_CRITICAL();
            vPortFree(driver);
            return 0;
        }
        prev = driver;
        driver = driver->next;
    }

    taskEXIT_CRITICAL();
    return -1;
}
