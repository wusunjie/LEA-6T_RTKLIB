#include "libusb.h"

#include <stdio.h>
#include <stdint.h>

#define LEA6T_USB_VENDOR_ID 0x1546
#define LEA6T_USB_PRODUCT_ID 0x01A6
#define LEA6T_USB_SERIAL_IF 1
#define LEA6T_USB_EP_TXD 0x01
#define LEA6T_USB_EP_RXD 0x82

struct serial_port {
    libusb_context *ctx;
    libusb_device_handle *handle;
};

static int open_serial_port(void);
static void close_serial_port(void);

static int read_serial_port(uint8_t *buffer, int len);
static int write_serial_port(uint8_t *buffer, int len);

static struct serial_port port;

extern struct device device_usb = {
    .open = open_serial_port,
    .close = close_serial_port,
    .read = read_serial_port,
    .write = write_serial_port
};

static int open_serial_port(void)
{
    if (LIBUSB_SUCCESS != libusb_init(&(port.ctx))) {
        return -1;
    }

    port.handle = libusb_open_device_with_vid_pid(port.ctx, LEA6T_USB_VENDOR_ID, LEA6T_USB_PRODUCT_ID);

    if (port.handle) {
        printf("LEA-6T Module open successfully\n");
        if (LIBUSB_SUCCESS == libusb_set_auto_detach_kernel_driver(port.handle, 1)) {
            printf("kernel driver will auto detach...\n");
        }
        if (LIBUSB_SUCCESS == libusb_claim_interface(port.handle, LEA6T_USB_SERIAL_IF)) {
            printf("claim interface successfully\n");
        }
        else {
            printf("claim interface failed\n");
            return -1;
        }
    }
    else {
        printf("LEA-6T Module not found\n");
        return -1;
    }

    return 0;
}

static void close_serial_port(void)
{
    libusb_release_interface(port.handle, LEA6T_USB_SERIAL_IF);
    libusb_close(port.handle);
    libusb_exit(port.ctx);
}

static int read_serial_port(uint8_t *buffer, int len)
{
    int actual = 0;
    int transferred = 0;
    while (len > transferred) {
        int ret = libusb_bulk_transfer(handle, LEA6T_USB_EP_RXD, buffer + transferred, len - transferred, &actual, 0);
        if (!ret) {
            transferred += actual;
        }
        else {
            printf("bulk transfer failed, %s\n", libusb_strerror(ret));
            break;
        }
    }

    return transferred;
}

static int write_serial_port(uint8_t *buffer, int len)
{
    int actual = 0;
    int transferred = 0;
    while (len > transferred) {
        int ret = libusb_bulk_transfer(handle, LEA6T_USB_EP_TXD, buffer + transferred, len - transferred, &actual, 0);
        if (!ret) {
            transferred += actual;
        }
        else {
            printf("bulk transfer failed, %s\n", libusb_strerror(ret));
            break;
        }
    }

    return transferred;
}

static int poll_serial_port(void)
{
    int ret = -1;
    struct libusb_pollfd **list = libusb_get_pollfds(port.ctx);
    if (list) {
        struct libusb_poll_fd *cur = NULL;
        int count = 0;
        for (cur = list[0]; cur != NULL; cur++) {
            count++;
        }
        if (count) {
            struct pollfd *polls = (struct pollfd *)calloc(count, sizeof(*polls));
            for (cur = list[0]; cur != NULL; cur++) {
                polls->fd = cur->fd;
                polls->events = cur->events;
            }
            ret = poll(polls, count, 0);
            if (ret > 0) {
                struct timeval timeout = {0, 0};
                ret = libusb_handle_events_timeout(port.ctx, timeout);
            }
            free(polls);
        }
    }

    return ret;
}