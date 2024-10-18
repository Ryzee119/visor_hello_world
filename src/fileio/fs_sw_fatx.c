#include "main.h"
#include <fcntl.h>
#include <unistd.h>

#include <fatx/libfatx/fatx.h>
#include <pc/ata.h>

static struct
{
    const char letter;
    uint64_t offset;
    uint64_t size;
    struct fatx_fs fs;
} partition_map[] = {
    {'C', 0x8ca80000, 0x01f400000, {}}, {'E', 0xabe80000, 0x131f00000, {}}, {'X', 0x00080000, 0x02ee00000, {}},
    {'Y', 0x2ee80000, 0x02ee00000, {}}, {'Z', 0x5dc80000, 0x02ee00000, {}},
};

typedef struct fatx_fs_file
{
    struct fatx_fs *fs;
    uint32_t cursor;
    struct fatx_attr attr;
    int flags;
    char path[255];
} fatx_fs_file_t;

typedef struct fatx_fs_dir
{
    struct fatx_fs *fs;
    struct fatx_dir dir;
    struct fatx_dirent *entry;
} fatx_fs_dir_t;

#define FATX_MIN(a, b) ((a) < (b) ? (a) : (b))

static struct fatx_fs *find_fatx_fs(const char drive_letter)
{
    for (uint8_t i = 0; i < sizeof(partition_map) / sizeof(partition_map[0]); i++) {
        if (partition_map[i].letter == drive_letter) {
            return &partition_map[i].fs;
        }
    }
    return NULL;
}

static int fatx_fs_init(const char drive_letter)
{
    char *path = "0:";
    path[0] = drive_letter;

    struct fatx_fs *fs = find_fatx_fs(drive_letter);
    if (fs == NULL) {
        return -1;
    }

    return fatx_open_device(fs, path, fs->partition_offset, fs->partition_size, 512, FATX_READ_FROM_SUPERBLOCK);
}

static int fatx_fs_open(const char *path, int flags)
{
    const char drive_letter = path[0];

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

static int fatx_fs_close(int fd)
{

    return 0;
}

static ssize_t fatx_fs_read(int fd, void *buffer, size_t count)
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

static ssize_t fatx_fs_write(int fd, const void *buffer, size_t count)
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

static off_t fatx_fs_lseek(int fd, off_t offset, int whence)
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

static void *fatx_fs_opendir(const char *path)
{
    struct fatx_fs *fs = find_fatx_fs(path[0]);
    if (fs == NULL) {
        return NULL;
    }

    fatx_fs_dir_t *fs_dir = pvPortMalloc(sizeof(fatx_fs_dir_t));
    if (fs_dir == NULL) {
        return NULL;
    }

    fs_dir->fs = fs;

    int result = fatx_open_dir(fs, path, &fs_dir->dir);
    if (result != FATX_STATUS_SUCCESS) {
        vPortFree(fs_dir);
        return NULL;
    }

    return fs_dir;
}

static fs_directory_entry_t *fatx_fs_readdir(void *handle, fs_directory_entry_t *entry)
{
    fatx_fs_dir_t *fs_dir = (fatx_fs_dir_t *)handle;
    struct fatx_dirent *result;
    struct fatx_attr attr;
    int status;

    do {
        status = fatx_read_dir(fs_dir->fs, &fs_dir->dir, fs_dir->entry, &attr, &result);
    } while (status == FATX_STATUS_FILE_DELETED);

    if (result == NULL || status == FATX_STATUS_END_OF_DIR || status != FATX_STATUS_SUCCESS) {
        return NULL;
    }

    entry->file_size = attr.file_size;
    strncpy(entry->file_name, attr.filename, sizeof(entry->file_name));
    return entry;
}

static void fatx_fs_closedir(void *handle)
{
    fatx_fs_dir_t *fs_dir = (fatx_fs_dir_t *)handle;
    fatx_close_dir(fs_dir->fs, &fs_dir->dir);
    vPortFree(fs_dir);
}

fs_sw_driver_t fs_sw_fatx_fs = {
    .init = fatx_fs_init,
    .open = fatx_fs_open,
    .close = fatx_fs_close,
    .read = fatx_fs_read,
    .write = fatx_fs_write,
    .lseek = fatx_fs_lseek,
    .opendir = fatx_fs_opendir,
    .readdir = fatx_fs_readdir,
    .closedir = fatx_fs_closedir,
};
