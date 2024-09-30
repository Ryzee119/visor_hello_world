#include "main.h"
#include <fcntl.h>
#include <unistd.h>

int open(const char *path, int flags, ...) {
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

ssize_t read(int fd, void *buf, size_t count) {
    FIL *fp = (FIL *)fd;
    UINT bytes_read;
    FRESULT result = f_read(fp, buf, count, &bytes_read);
    return (result == FR_OK) ? bytes_read : -1;
}

ssize_t write(int fd, const void *buf, size_t count) {
    FIL *fp = (FIL *)fd;
    UINT bytes_written;
    FRESULT result = f_write(fp, buf, count, &bytes_written);
    return (result == FR_OK) ? bytes_written : -1;
}

int close(int fd) {
    FIL *fp = (FIL *)fd;
    f_close(fp);
    vPortFree(fp);
    return 0;
}

off_t lseek(int fd, off_t offset, int whence) {
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

static fileio_drv_t* fileio_driver[FF_VOLUMES] = {0};

int8_t fileio_register_driver(uint8_t pdrv, fileio_drv_t *driver)
{
    if (fileio_driver[pdrv] != NULL) {
        return -1;
    }
    fileio_driver[pdrv] = driver;
    return 0;
}

int8_t fileio_unregister_driver(uint8_t pdrv)
{
    fileio_driver[pdrv] = NULL;
    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (fileio_driver[pdrv] == NULL) {
        return STA_NOINIT;
    }
    return fileio_driver[pdrv]->disk_status(pdrv);

}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (fileio_driver[pdrv] == NULL) {
        return STA_NODISK;
    }
    return fileio_driver[pdrv]->disk_initialize(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (fileio_driver[pdrv] == NULL) {
        return RES_ERROR;
    }
    return fileio_driver[pdrv]->disk_read(pdrv, buff, sector, count);
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (fileio_driver[pdrv] == NULL) {
        return RES_ERROR;
    }
    return fileio_driver[pdrv]->disk_write(pdrv, buff, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (fileio_driver[pdrv] == NULL) {
        return RES_ERROR;
    }
    return fileio_driver[pdrv]->disk_ioctl(pdrv, cmd, buff);
}
