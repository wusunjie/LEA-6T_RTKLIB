#include "device.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct event_source {
    struct pollfd *list;
    int len;
};


extern struct device device_usb;

static int status = 0;

static struct device *dev = &device_usb;

static struct event_source usb_source;

int navi_start(void)
{
    if (!dev->open()) {
        usb_source.list = NULL;
        usb_source.len = dev->get_pollfd(&(usb_source.list));
        status = 0;
        return 0;
    }

    return -1;
}

void navi_stop(void)
{
    dev->close();
    free(usb_source.list);
}

int navi_event_handle(void)
{
    struct pollfd *fds = (struct pollfd *)malloc(usb_source.len * sizeof(*fds));
    memcpy(fds, usb_source.list, usb_source.len);

    int len = usb_source.len;

    if (len > 0) {
        int ret = poll(fds, len, -1);
        if (ret > 0) {
            int i, j, usb_source_flag = 0;
            for (i = 0; i < len; i++) {
                if (!fds[i].revents) {
                    continue;
                }
                for (j = 0; j < len; j++) {
                    if (usb_source.list[j].fd == fds[i].fd) {
                        usb_source_flag = 1;
                        break;
                    }
                }
            }

            if (usb_source_flag) {
                dev->handle_event();
            }
        }
    }

    free(fds);

    return 0;
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
