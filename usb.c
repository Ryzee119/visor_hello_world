#include "main.h"
void tusb_host_task(void *parameters)
{
    (void)parameters;
    tusb_init();
    while (1) {
        tuh_task();
    }
}

void usb0_handler()
{
    hcd_int_handler(TUH_OPT_RHPORT, true);
}

void usb_init()
{
    xTaskCreate(tusb_host_task, "tusb_host_task", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_NORMAL, NULL);
}

void hcd_int_enable(uint8_t rhport)
{
    pic8259_irq_enable(XBOX_PIC1_DATA_PORT, PCI_USB0_IRQ);
}

void hcd_int_disable(uint8_t rhport)
{
    pic8259_irq_disable(XBOX_PIC1_DATA_PORT, PCI_USB0_IRQ);
}

void tuh_mount_cb(uint8_t daddr)
{
    TU_LOG2("[USB] USB Device %u Mounted\n", daddr);
}

void tuh_umount_cb(uint8_t daddr)
{
    TU_LOG2("[USB] USB Device %u Unmounted\n", daddr);
}

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count){
    *driver_count = 1;
    return &usbh_xinput_driver;
}

void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly);
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len)
{
    const xinput_gamepad_t *p = &xid_itf->pad;
    const char* type_str;

    if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS)
    {
        switch (xid_itf->type)
        {
            case 1: type_str = "Xbox One";          break;
            case 2: type_str = "Xbox 360 Wireless"; break;
            case 3: type_str = "Xbox 360 Wired";    break;
            case 4: type_str = "Xbox OG";           break;
            default: type_str = "Unknown";
        }

        if (xid_itf->connected && xid_itf->new_pad_data)
        {
           // TU_LOG1("[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
           //     dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);

            //How to check specific buttons
           // if (p->wButtons & XINPUT_GAMEPAD_A) TU_LOG1("You are pressing A\n");
           dooom_new_input(p->wButtons, p->sThumbLX, p->sThumbLY);
        }
    }
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    TU_LOG1("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
    {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }
    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 1, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    TU_LOG1("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}
