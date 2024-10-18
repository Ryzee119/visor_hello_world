#include "main.h"
#include <fcntl.h>
#include <unistd.h>

static int fatfs_init(const char drive_letter)
{
    return 0;
}

static int fatfs_open(const char *path, int flags)
{
    BYTE mode = 0;
    if ((flags & O_RDONLY) == O_RDONLY) {
        mode = FA_READ;
    } else if ((flags & O_WRONLY) == O_WRONLY) {
        mode = FA_WRITE;
    } else if ((flags & O_RDWR) == O_RDWR) {
        mode = FA_READ | FA_WRITE;
    }
    if (flags & O_CREAT) {
        mode |= FA_CREATE_ALWAYS;
    }
    FIL *fp = pvPortMalloc(sizeof(FIL));
    FRESULT result = f_open(fp, path, mode);
    return (result == FR_OK) ? (int)fp : -1;
}

static int fatfs_close(int fd)
{
    FIL *fp = (FIL *)fd;
    f_close(fp);
    vPortFree(fp);
    return 0;
}

static ssize_t fatfs_read(int fd, void *buffer, size_t count)
{
    FIL *fp = (FIL *)fd;
    UINT bytes_read;
    FRESULT result = f_read(fp, buffer, count, &bytes_read);
    return (result == FR_OK) ? bytes_read : -1;
}

static ssize_t fatfs_write(int fd, const void *buffer, size_t count)
{
    FIL *fp = (FIL *)fd;
    UINT bytes_written;
    FRESULT result = f_write(fp, buffer, count, &bytes_written);
    return (result == FR_OK) ? bytes_written : -1;
}

static off_t fatfs_lseek(int fd, off_t offset, int whence)
{
    FIL *fp = (FIL *)fd;
    FSIZE_t file_end = f_size(fp);
    FSIZE_t file_current = f_tell(fp);

    DWORD adjusted_offset = 0;
    if (whence == SEEK_SET) {
        adjusted_offset = offset;
    } else if (whence == SEEK_CUR) {
        adjusted_offset = file_current + offset;
    } else if (whence == SEEK_END) {
        adjusted_offset = file_end + offset;
    }

    FRESULT result = f_lseek(fp, adjusted_offset);
    return (result == FR_OK) ? adjusted_offset : -1;
}

static void *fatfs_opendir(const char *path)
{
    DIR *dir = pvPortMalloc(sizeof(DIR));
    if (dir == NULL) {
        return NULL;
    }

    FRESULT result = f_opendir(dir, path);
    if (result != FR_OK) {
        vPortFree(dir);
        return NULL;
    }

    return (result == FR_OK) ? dir : NULL;
}

static fs_directory_entry_t *fatfs_readdir(void *handle, fs_directory_entry_t *entry)
{
    FILINFO fno;
    FRESULT result = f_readdir((DIR *)handle, &fno);
    if (result != FR_OK || fno.fname[0] == 0) {
        return NULL;
    }

    entry->file_size = fno.fsize;
    strncpy(entry->file_name, fno.fname, sizeof(entry->file_name));
    return entry;
}

static void fatfs_closedir(void *handle)
{
    f_closedir((DIR *)handle);
    vPortFree(handle);
}

fs_sw_driver_t fs_sw_fatfs = {
    .init = fatfs_init,
    .open = fatfs_open,
    .close = fatfs_close,
    .read = fatfs_read,
    .write = fatfs_write,
    .lseek = fatfs_lseek,
    .opendir = fatfs_opendir,
    .readdir = fatfs_readdir,
    .closedir = fatfs_closedir,
};

// Callbacks from internal FATFS driver
DSTATUS disk_status(BYTE pdrv)
{
    char drive_letter = '0' + pdrv;
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return STA_NOINIT;
    }

    int8_t status = driver->hw->disk_status(drive_letter);
    return (status == 0) ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    char drive_letter = '0' + pdrv;
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return STA_NOINIT;
    }

    int8_t status = driver->hw->disk_initialize(drive_letter);
    if (status != 0) {
        printf_r("[FATFS] disk_initialize failed with status %d\n", status);
    }
    return (status == 0) ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    char drive_letter = '0' + pdrv;
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    int8_t status = driver->hw->disk_read(drive_letter, buff, sector, count);
    if (status != 0) {
        printf_r("[FATFS] disk_read failed with status %d\n", status);
    }
    return (status == 0) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    char drive_letter = '0' + pdrv;
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    int8_t status = driver->hw->disk_write(drive_letter, buff, sector, count);
    if (status != 0) {
        printf_r("[FATFS] disk_write failed with status %d\n", status);
    }
    return (status == 0) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    char drive_letter = '0' + pdrv;
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    int8_t status = driver->hw->disk_ioctl(drive_letter, cmd, buff);
    if (status != 0) {
        printf_r("[FATFS] disk_ioctl failed with status %d\n", status);
    }
    return (status == 0) ? RES_OK : RES_ERROR;
}
