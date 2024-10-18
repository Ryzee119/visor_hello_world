#include "main.h"

#define MSC_DEBUG
#ifdef MSC_DEBUG
#define MSC_PRINTF printf_r
#else
#define MSC_PRINTF(...)
#endif

typedef struct usb_msc_device
{
    uint8_t dev_addr;
    uint8_t driver_letter;
    scsi_inquiry_resp_t inquiry_response;
    FATFS *fatfs;
} usb_msc_device_t;
static usb_msc_device_t msc_device[CFG_TUH_MSC];

static bool usb_transfer_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
{
    int *finished = (int *)cb_data->user_arg;
    msc_csw_t const *csw = cb_data->csw;

    if (csw->status == 0) {
        *finished = 1;
    } else {
        printf_r("[USBMSC] SCSI command failed with status %d\n", csw->status);
        *finished = -1;
    }

    return true;
}

static void msc_mount_task(void *parameters)
{
    usb_msc_device_t *msc = parameters;
    msc->fatfs = pvPortMalloc(sizeof(FATFS));

    if (msc->fatfs == NULL) {
        MSC_PRINTF("[USBMSC] Could not allocate memory for FATFS\n");
        vTaskDelete(NULL);
        return;
    }


    // Register the USB driver with the FATFS driver then attempt to mount the drive
    extern fs_hw_driver_t fs_hw_usb;
    extern fs_sw_driver_t fs_sw_fatfs;
    extern fs_sw_driver_t fs_sw_fatx_fs;
    fileio_register_driver(msc->driver_letter, &fs_hw_usb, &fs_sw_fatx_fs);

    char drive_path[4];
    sprintf(drive_path, "%c:", msc->driver_letter);

    // Attempt to mount USB as FATFS
    FRESULT res = f_mount(msc->fatfs, drive_path, 1);
    if (res != FR_OK) {
        MSC_PRINTF("[USBMSC] Could not mount drive %c, res %d\n", msc->driver_letter, res);
        fileio_unregister_driver(msc->driver_letter);
    }
    vTaskDelete(NULL);
    return;
}

void tuh_msc_mount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == 0) {
            msc_device[i].dev_addr = dev_addr;
            msc_device[i].driver_letter = '0' + i;
            xTaskCreate(msc_mount_task, "msc_mount_task", configMINIMAL_STACK_SIZE, &msc_device[i],
                        THREAD_PRIORITY_NORMAL, NULL);
            return;
        }
    }
    MSC_PRINTF("[USBMSC] No space to store device\r\n");
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr != dev_addr) {
            continue;
        }

        if (msc_device[i].fatfs == NULL) {
            return;
        }

        if (msc_device[i].fatfs->fs_type != 0) {
            char drive_path[4];
            sprintf(drive_path, "%c:", msc_device[i].driver_letter);
            f_unmount((const TCHAR *)drive_path);
        }

        vPortFree(msc_device[i].fatfs);
        msc_device[i].fatfs = NULL;
        msc_device[i].dev_addr = 0;
        MSC_PRINTF("[USBMSC] Address %d unmounted from index %d\r\n", dev_addr, i);
        return;
    }
}

static int8_t usb_disk_status(char drive_letter)
{
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }
    return 0;
}

static int8_t usb_disk_initialize(char drive_letter)
{
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }
    return 0;
}

static int8_t usb_disk_read(char drive_letter, void *buffer, uint64_t sector, uint32_t count)
{
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }

    uint8_t pdrv = drive_letter - '0';
    const usb_msc_device_t *msc = &msc_device[pdrv];
    int finished = 0;
    if (tuh_msc_read10(msc->dev_addr, 0, buffer, sector, (uint16_t)count, usb_transfer_cb, (uintptr_t)&finished)) {
        while (finished == 0) {
            system_yield(0);
        }
        if (finished == -1) {
            printf_r("disk_read failed, sector %d, count %d\n", sector, count);
            return -1;
        }
        return 0;
    }
    return -1;
}

static int8_t usb_disk_write(char drive_letter, const void *buffer, uint64_t sector, uint32_t count)
{
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }

    uint8_t pdrv = drive_letter - '0';
    const usb_msc_device_t *msc = &msc_device[pdrv];
    int finished = 0;
    if (tuh_msc_write10(msc->dev_addr, 0, buffer, sector, (uint16_t)count, usb_transfer_cb, (uintptr_t)&finished)) {
        while (finished == 0) {
            system_yield(0);
        }
        if (finished == -1) {
            printf_r("disk_write failed\n");
            return -1;
        }
        return 0;
    }
    return -1;
}

static int8_t usb_disk_ioctl(char drive_letter, uint8_t cmd, void *buff)
{
    file_system_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }

    uint8_t pdrv = drive_letter - '0';
    const usb_msc_device_t *msc = &msc_device[pdrv];

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD *)buff) = (WORD)tuh_msc_get_block_count(msc->dev_addr, 0);
            return RES_OK;
        case GET_SECTOR_SIZE:
            *((WORD *)buff) = (WORD)tuh_msc_get_block_size(msc->dev_addr, 0);
            return RES_OK;
        case GET_BLOCK_SIZE:
            *((DWORD *)buff) = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }

    return 0;
}

fs_hw_driver_t fs_hw_usb = {
    .disk_status = usb_disk_status,
    .disk_initialize = usb_disk_initialize,
    .disk_read = usb_disk_read,
    .disk_write = usb_disk_write,
    .disk_ioctl = usb_disk_ioctl,
};
