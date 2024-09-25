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
    uint8_t drive_number;
    uint8_t drive_path[4];
    scsi_inquiry_resp_t inquiry_response;
    FATFS *fatfs;

} usb_msc_device_t;

usb_msc_device_t msc_device[CFG_TUH_MSC];

void fileio_usb_init()
{
    memset(msc_device, 0, sizeof(msc_device));
}

static bool msc_transfer_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
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

    msc->drive_path[0] = '\0';
    msc->fatfs = pvPortMalloc(sizeof(FATFS));
    if (msc->fatfs == NULL) {
        printf_r("[USBMSC] Could not allocate memory for FATFS\n");
        vTaskDelete(NULL);
        return;
    }

    FRESULT res = f_mount(msc->fatfs, (const TCHAR *)msc->drive_path, 1);
    if (res != FR_OK) {
        printf_r("[USBMSC] f_mount failed with error %d\r\n", res);
        vTaskDelete(NULL);
        return;
    }
    printf_r("[USBMSC] Disk mounted as drive %s\r\n", msc->drive_path);

    // Alert doom we are ready
    extern SemaphoreHandle_t doom_mutex;
    xSemaphoreGive(doom_mutex);
    vTaskDelete(NULL);
}

void tuh_msc_mount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == 0) {
            msc_device[i].dev_addr = dev_addr;
            msc_device[i].drive_number = i;
            msc_device[i].fatfs = NULL;
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
        if (msc_device[i].dev_addr == dev_addr) {
            if (msc_device[i].fatfs != NULL) {
                if (msc_device[i].fatfs->fs_type != 0) {
                    f_unmount((const TCHAR *)msc_device[i].drive_path);
                }
                vPortFree(msc_device[i].fatfs);
                msc_device[i].fatfs = NULL;
            }
            msc_device[i].dev_addr = 0;
            MSC_PRINTF("[USBMSC] Address %d unmounted from index %d\r\n", dev_addr, i);
            return;
        }
    }
}

/* FATFS WRAPPER FOR USB IO */
DSTATUS disk_status(BYTE pdrv)
{
    uint8_t dev_addr = pdrv + 1;
    return tuh_msc_mounted(dev_addr) ? 0 : STA_NODISK;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    const usb_msc_device_t *msc = &msc_device[pdrv];
    int finished = 0;
    if (tuh_msc_read10(msc->dev_addr, 0, buff, sector, (uint16_t)count, msc_transfer_cb, (uintptr_t)&finished)) {
        while (finished == 0) {
            taskYIELD();
        }
        if (finished == -1) {
            printf_r("disk_read failed, sector %d, count %d\n", sector, count);
            return RES_ERROR;
        }
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    const usb_msc_device_t *msc = &msc_device[pdrv];
    int finished = 0;
    if (tuh_msc_write10(msc->dev_addr, 0, buff, sector, (uint16_t)count, msc_transfer_cb, (uintptr_t)&finished)) {
        while (finished == 0) {
            taskYIELD();
        }
        if (finished == -1) {
            printf_r("disk_write failed\n");
            return RES_ERROR;
        }
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    const usb_msc_device_t *msc = &msc_device[pdrv];
    uint8_t dev_addr = msc->dev_addr;
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *((DWORD *)buff) = (WORD)tuh_msc_get_block_count(dev_addr, 0);
        return RES_OK;
    case GET_SECTOR_SIZE:
        *((WORD *)buff) = (WORD)tuh_msc_get_block_size(dev_addr, 0);
        return RES_OK;
    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
