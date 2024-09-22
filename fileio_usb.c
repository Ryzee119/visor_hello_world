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

static usb_msc_device_t *get_msc_device(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == dev_addr) {
            return &msc_device[i];
        }
    }
    return NULL;
}

static bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
{
    msc_cbw_t const *cbw = cb_data->cbw;
    msc_csw_t const *csw = cb_data->csw;

    if (csw->status != 0) {
        MSC_PRINTF("Inquiry failed\r\n");
        return false;
    }

    usb_msc_device_t *msc = get_msc_device(dev_addr);
    if (msc == NULL) {
        return false;
    }

    // Print out Vendor ID, Product ID and Rev
    MSC_PRINTF("[USBMSC] %.8s %.16s rev %.4s\r\n", msc->inquiry_response.vendor_id, msc->inquiry_response.product_id, msc->inquiry_response.product_rev);

    // Get capacity of device
    uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
    uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cbw->lun);

    MSC_PRINTF("[USBMSC] Disk Size: %" PRIu32 " MB\r\n", block_count / ((1024 * 1024) / block_size));

    msc->drive_path[0] = '0' + msc->drive_number;
    msc->drive_path[1] = ':';
    msc->drive_path[2] = '\0';

    msc->fatfs = pvPortMalloc(sizeof(FATFS));
    if (msc->fatfs == NULL) {
        MSC_PRINTF("[USBMSC] Could not allocate memory for FATFS\n");
        return false;
    }

    MSC_PRINTF("[USBMSC] Trying to mount to %s\r\n", msc->drive_path);

    FRESULT res = f_mount(msc->fatfs, msc->drive_path, 1);
    if (res != FR_OK) {
        MSC_PRINTF("[USBMSC] f_mount failed with error %d\r\n", res);
    }
    MSC_PRINTF("[USBMSC] Disk mounted as drive %s\r\n", msc->drive_path);

    // test fopen
    //static FIL *file;
    //file = fopen("0:/test.txt", "r");
    return true;
}

void tuh_msc_mount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == 0) {
            msc_device[i].dev_addr = dev_addr;
            msc_device[i].drive_number = i;
            msc_device[i].fatfs = NULL;
            tuh_msc_inquiry(dev_addr, 0, &msc_device[i].inquiry_response, inquiry_complete_cb, 0);
            MSC_PRINTF("[USBMSC] Address %d mounted at index %d\r\n", dev_addr, i);
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
                    f_unmount(msc_device[i].drive_path);
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

DSTATUS disk_status(BYTE pdrv)
{
    uint8_t dev_addr = pdrv + 1;
    return tuh_msc_mounted(dev_addr) ? 0 : STA_NODISK;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return 0;
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
{
    uint8_t *finished = (uint8_t *)cb_data->user_arg;
    *finished = 1;
    return true;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{

    const usb_msc_device_t *msc = &msc_device[pdrv];
    uint8_t finished = 0;
    // MSC_PRINTF("[USBMSC] READ pdrv: %d, dev_addr: %02x, sector: %d, count: %d\n", pdrv, msc->dev_addr, sector, count);
    if (tuh_msc_read10(msc->dev_addr, 0, buff, sector, (uint16_t)count, disk_io_complete, (uintptr_t)&finished)) {
        while (finished == 0) {
            taskYIELD();
            tuh_task();
        }
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    const usb_msc_device_t *msc = &msc_device[pdrv];
    uint8_t finished = 0;
    // MSC_PRINTF("[USBMSC] WRITE pdrv: %d, dev_addr: %02x, sector: %d, count: %d\n", pdrv, msc->dev_addr, sector, count);
    if (tuh_msc_write10(msc->dev_addr, 0, buff, sector, (uint16_t)count, disk_io_complete, (uintptr_t)&finished)) {
        while (finished == 0) {
            taskYIELD();
            tuh_task();
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
