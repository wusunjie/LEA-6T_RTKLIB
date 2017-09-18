#include "device.h"

extern struct device device_usb;

struct device *dev = &device_usb;

int navi_start(void)
{
    return dev->open();
}

int parse_ublox_message(uint8_t *data, int len)
{
    return -1;
}