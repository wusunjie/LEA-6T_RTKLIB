#include "device.h"

extern struct device device_usb;

static struct device *device_handle = NULL;

int liblea6t_open(void)
{
    if (0 == device_usb.open()) {
        device_handle = &device_usb;
        return 0;
    }

    return -1;
}

void liblea6t_close(void)
{
    if (device_handle) {
        device_handle->close();
    }
}