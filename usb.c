#include "main.h"
void tusb_host_task(void *parameters)
{
    (void)parameters;
    tusb_init();
    while (1) {
        tuh_task();
    }
}

SemaphoreHandle_t usb_irq_sem;
void usb0_handler()
{
    mmio_output_word(PCI_USB0_MEMORY_REGISTER_BASE_0 + (20), 0x80000000);
    xSemaphoreGiveFromISR(usb_irq_sem, NULL);
}

void usb0_deferred_handler(void *parameters)
{
    (void)parameters;
    while (1) {
        xSemaphoreTake(usb_irq_sem, portMAX_DELAY);
        hcd_int_handler(TUH_OPT_RHPORT, false);
        uint32_t irq_status = mmio_input_word(PCI_USB0_MEMORY_REGISTER_BASE_0 + 16);
        mmio_output_word(PCI_USB0_MEMORY_REGISTER_BASE_0 + 16, 0x80000000 | irq_status);
    }
}

void usb_init()
{
    usb_irq_sem = xSemaphoreCreateCounting(255, 0);
    xTaskCreate(tusb_host_task, "tusb_host_task", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_NORMAL, NULL);
    xTaskCreate(usb0_deferred_handler, "usb0_deferred_handler", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_HIGH, NULL);
}

void hcd_int_enable(uint8_t rhport)
{
    pic8259_irq_enable(XBOX_PIC1_DATA_PORT, PCI_USB0_IRQ);
}

void hcd_int_disable(uint8_t rhport)
{
    pic8259_irq_disable(XBOX_PIC1_DATA_PORT, PCI_USB0_IRQ);
}

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count){
    *driver_count = 1;
    return &usbh_xinput_driver;
}

void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly);
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len)
{
    const xinput_gamepad_t *p = &xid_itf->pad;
  
    if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS)
    {

        if (xid_itf->connected && xid_itf->new_pad_data)
        {
           dooom_new_input(p->wButtons, p->sThumbLX, p->sThumbLY);
        }
    }
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    TU_LOG1("[INPUT] XINPUT MOUNTED %02x %d\n", dev_addr, instance);

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
    TU_LOG1("[INPUT] XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}
