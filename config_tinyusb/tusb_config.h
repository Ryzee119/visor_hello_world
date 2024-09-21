#define CFG_TUH_ENABLED 1
#define CFG_TUH_MAX_SPEED OPT_MODE_FULL_SPEED
#define TUP_USBIP_OHCI
#define TUP_DCD_ENDPOINT_MAX 0
#define CFG_TUSB_OS OPT_OS_FREERTOS

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 1
#endif

#ifndef CFG_TUH_ENUMERATION_BUFSIZE
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#endif

#ifndef CFG_TUH_HUB
#define CFG_TUH_HUB 2
#endif

//Not including USB Hubs
#ifndef CFG_TUH_DEVICE_MAX 
#define CFG_TUH_DEVICE_MAX 2
#endif

#ifndef TUP_OHCI_RHPORTS
#define TUP_OHCI_RHPORTS 4
#endif

#ifndef CFG_TUH_XINPUT
#define CFG_TUH_XINPUT 1
#endif

#ifndef CFG_TUH_MSC
#define CFG_TUH_MSC 1
#endif

#define CFG_TUSB_DEBUG_PRINTF printf_r