#include "main.h"
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include <libfatx/fatx.h>
#include <libfatx/fatx_internal.h>
#include <pc/ata.h>

static struct
{
    const char letter;
    uint64_t offset;
    uint64_t size;
    uint8_t initialised;
    struct fatx_fs fs;
} partition_map[] = {
    {'C', 0x8ca80000, 0x01f400000, 0, {}}, {'E', 0xabe80000, 0x131f00000, 0, {}}, {'X', 0x00080000, 0x02ee00000, 0, {}},
    {'Y', 0x2ee80000, 0x02ee00000, 0, {}}, {'Z', 0x5dc80000, 0x02ee00000, 0, {}},
};

typedef struct fatx_fs_file
{
    struct fatx_fs *fs;
    uint32_t cursor;
    struct fatx_attr attr;
    int flags;
    char path[255];
    uint8_t sector_cache[ATA_SECTOR_SIZE];
} fatx_fs_file_t;

typedef struct fatx_fs_dir
{
    struct fatx_fs *fs;
    struct fatx_dir dir;
    struct fatx_dirent *entry;
} fatx_fs_dir_t;

#define FATX_MIN(a, b) ((a) < (b) ? (a) : (b))

static struct fatx_fs *find_fatx_fs(char drive_letter)
{
    drive_letter = toupper(drive_letter);
    for (uint8_t i = 0; i < sizeof(partition_map) / sizeof(partition_map[0]); i++) {
        if (partition_map[i].letter == drive_letter) {
            if (partition_map[i].initialised == 0) {

                partition_map[i].initialised = 1;
                char path[3] = {'/', partition_map[i].letter, '\0'};
                fatx_open_device(&partition_map[i].fs, path, partition_map[i].offset, partition_map[i].size, 512,
                                 FATX_READ_FROM_SUPERBLOCK);
            }
            return &partition_map[i].fs;
        }
    }
    return NULL;
}

int open(const char *path, int flags, ...)
{
    char drive_letter = path[1];

    struct fatx_fs *fs = find_fatx_fs(drive_letter);
    if (fs == NULL) {
        return -1;
    }

    fatx_fs_file_t *handle = pvPortMalloc(sizeof(fatx_fs_file_t));
    if (handle == NULL) {
        return -1;
    }
    handle->fs = fs;
    handle->cursor = 0;
    handle->flags = flags;
    strncpy(handle->path, path, sizeof(handle->path));

    if (flags & O_CREAT) {
        if (fatx_mknod(fs, path) != FATX_STATUS_SUCCESS) {
            vPortFree(handle);
            return -1;
        }
    }

    if (fatx_get_attr(fs, path, &handle->attr) != FATX_STATUS_SUCCESS) {
        vPortFree(handle);
        return -1;
    }

    // Readonly file opened for Write
    if ((flags & O_RDWR || flags & O_WRONLY) && (handle->attr.attributes & FATX_ATTR_READ_ONLY)) {
        vPortFree(handle);
        return -1;
    }

    // On append, cursor moved to end of the file
    if (flags & O_APPEND) {
        handle->cursor = handle->attr.file_size;
    }

    handle->flags = flags;

    return (int)handle;
}

int close(int fd)
{
    fatx_fs_file_t *handle = (fatx_fs_file_t *)fd;
    vPortFree(handle);
    return 0;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    fatx_fs_file_t *handle = (fatx_fs_file_t *)fd;
    struct fatx_fs *fs = handle->fs;
    if (fs == NULL) {
        return -1;
    }

    if (handle->flags & O_APPEND) {
        return -1;
    }

    int bytes_transferred = fatx_read(fs, handle->path, handle->cursor, count, buffer);
    if (bytes_transferred > 0) {
        handle->cursor += bytes_transferred;
    }

    return bytes_transferred;
}

ssize_t write(int fd, const void *buffer, size_t count)
{
    fatx_fs_file_t *handle = (fatx_fs_file_t *)fd;
    struct fatx_fs *fs = handle->fs;
    if (fs == NULL) {
        return -1;
    }

    if (handle->flags & O_RDONLY) {
        return -1;
    }

    int bytes_transferred = fatx_write(fs, handle->path, handle->cursor, count, buffer);
    if (bytes_transferred > 0) {
        handle->cursor += bytes_transferred;
    }

    return bytes_transferred;
}

off_t lseek(int fd, off_t offset, int whence)
{
    fatx_fs_file_t *handle = (fatx_fs_file_t *)fd;
    struct fatx_fs *fs = handle->fs;
    if (fs == NULL) {
        return -1;
    }

    uint32_t file_end = handle->attr.file_size;

    DWORD adjusted_offset = 0;
    if (whence == SEEK_SET) {
        adjusted_offset = offset;
    } else if (whence == SEEK_CUR) {
        adjusted_offset = handle->cursor + offset;
    } else if (whence == SEEK_END) {
        adjusted_offset = file_end + offset;
    }

    // Pad out the file with zeroes if seek past end
    if (adjusted_offset > file_end && (handle->flags & O_WRONLY || handle->flags & O_RDWR)) {
        uint32_t bytes_to_write = adjusted_offset - file_end;
        uint32_t chunk = FATX_MIN(bytes_to_write, 512);
        uint8_t *zeroes = pvPortMalloc(chunk);
        if (zeroes == NULL) {
            return -1;
        }
        memset(zeroes, 0, chunk);
        while (bytes_to_write) {
            uint32_t bytes_written = fatx_write(fs, handle->path, file_end, chunk, zeroes);
            if (bytes_written == 0) {
                vPortFree(zeroes);
                return -1;
            }
            file_end += bytes_written;
            bytes_to_write -= bytes_written;
        }
        vPortFree(zeroes);

        // Re-get attributes with new file size
        fatx_get_attr(handle->fs, handle->path, &handle->attr);
    }

    handle->cursor = adjusted_offset;
    return adjusted_offset;
}

directory_handle_t *opendir(const char *path)
{
    struct fatx_fs *fs = find_fatx_fs(path[0]);
    if (fs == NULL) {
        return NULL;
    }

    path += 2;

    directory_handle_t *dir = pvPortMalloc(sizeof(directory_handle_t) + sizeof(fatx_fs_dir_t));
    if (dir == NULL) {
        return NULL;
    }

    dir->user_data = (void *)dir + sizeof(directory_handle_t);

    fatx_fs_dir_t *fs_dir = (fatx_fs_dir_t *)dir->user_data;

    fs_dir->fs = fs;

    int result = fatx_open_dir(fs, path, &fs_dir->dir);
    if (result != FATX_STATUS_SUCCESS) {
        vPortFree(dir);
        return NULL;
    }

    return dir;
}

directory_entry_t *readdir(directory_handle_t *dir)
{
    fatx_fs_dir_t *fs_dir = (fatx_fs_dir_t *)dir->user_data;
    struct fatx_dirent *result;
    struct fatx_attr attr;
    int status;

    result = pvPortMalloc(sizeof(struct fatx_dirent));
    if (result == NULL) {
        return NULL;
    }

    do {
        status = fatx_read_dir(fs_dir->fs, &fs_dir->dir, fs_dir->entry, &attr, &result);
        fatx_next_dir_entry(fs_dir->fs, &fs_dir->dir);
    } while (status == FATX_STATUS_FILE_DELETED);

    if (status != FATX_STATUS_SUCCESS) {
        vPortFree(result);
        return NULL;
    }
    directory_entry_t *entry = &dir->entry;
    entry->file_size = attr.file_size;
    strncpy(entry->file_name, result->filename, sizeof(entry->file_name));
    return entry;
}

void closedir(directory_handle_t *dir)
{
    fatx_fs_dir_t *fs_dir = (fatx_fs_dir_t *)dir->user_data;
    fatx_close_dir(fs_dir->fs, &fs_dir->dir);
    vPortFree(dir);
}

static uint64_t seek_offset = 0;
static uint8_t sector_cache[ATA_SECTOR_SIZE];
static uint64_t cached_lba = 0xFFFFFFFFFFFFFFFF;

int fatx_dev_seek(struct fatx_fs *fs, uint64_t offset)
{
    seek_offset = offset;
    return 0;
}

int fatx_dev_seek_cluster(struct fatx_fs *fs, size_t cluster, off_t offset)
{
    int status;
    uint64_t pos;

    status = fatx_cluster_number_to_byte_offset(fs, cluster, &pos);
    if (status) {
        return status;
    }

    pos += offset;

    return fatx_dev_seek(fs, pos);
}

extern ata_bus_t ata_bus;
size_t fatx_dev_read(struct fatx_fs *fs, void *buf, size_t size, size_t items)
{
    const uint32_t sector_size = fs->sector_size;
    const uint32_t lba_offset = seek_offset % sector_size;
    uint64_t current_lba = seek_offset / sector_size;
    uint64_t bytes_remaining = size * items;

    uint8_t *buf8 = (uint8_t *)buf;

    // Unaligned read for first sector
    if (lba_offset != 0) {
        uint32_t chunk = FATX_MIN(bytes_remaining, sector_size - lba_offset);
        ide_dma_read(&ata_bus, 0, current_lba, sector_cache, 1);
        memcpy(buf8, sector_cache + lba_offset, chunk);

        bytes_remaining -= chunk;
        buf8 += chunk;
        current_lba++;
        seek_offset += chunk;
    }

    // Sector aligned read
    if (bytes_remaining >= sector_size && bytes_remaining % sector_size == 0) {
        uint32_t full_sectors = bytes_remaining / sector_size;
        uint64_t full_bytes = full_sectors * sector_size;
        ide_dma_read(&ata_bus, 0, current_lba, buf, full_sectors);

        bytes_remaining -= full_bytes;
        buf8 += full_bytes;
        current_lba += full_sectors;
        seek_offset += full_bytes;
    }

    // Partial read for last sector
    if (bytes_remaining > 0) {
        ide_dma_read(&ata_bus, 0, current_lba, sector_cache, 1);
        memcpy(buf8, sector_cache, bytes_remaining);
        seek_offset += bytes_remaining;
        bytes_remaining = 0;
    }

    return items;
}

size_t fatx_dev_write(struct fatx_fs *fs, const void *buf, size_t size, size_t items)
{

    printf("WRITE\n");
    return 0;
}