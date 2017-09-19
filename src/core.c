#include "device.h"

#include <stdio.h>
#include <string.h>

extern struct device device_usb;

static int status = 0;

struct device *dev = &device_usb;

int navi_start(void)
{
    return dev->open();
}

int navi_event_handle(void)
{
    return dev->poll();
}

int parse_ublox_message(uint8_t *data, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        printf("%c", data[i]);
    }
    printf("\n");
    if (10 == status) {
        uint8_t buffer[] = "$PUBX,41,3,0001,0001,9600,1*%x\r\n";
        uint8_t data[255] = {0};
        uint8_t cs = buffer[1];
        for (i = 2; i < 27; i++) {
            cs ^= buffer[i];
        }
        snprintf(data, 255, buffer, cs);
        dev->write(data, strlen(data));
    }
    else if (9 == status) {
        uint8_t buffer[11] = {0xB5, 0x62, 0x06, 0x01, 0x00, 0x03,
            0xF1,
            0x41,
            0x01};
        for (i = 2; i < 9; i++) {
            buffer[9] += buffer[i];
            buffer[10] += buffer[9];
        }
        dev->write(buffer, sizeof(buffer));
    }

    status++;

    return len;
}