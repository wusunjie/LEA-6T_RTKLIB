#include "libusb.h"

#include "device.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>

#define LEA6T_USB_VENDOR_ID 0x1546
#define LEA6T_USB_PRODUCT_ID 0x01A6
#define LEA6T_USB_SERIAL_IF 1
#define LEA6T_USB_EP_TXD 0x01
#define LEA6T_USB_EP_RXD 0x82

struct serial_port {
    libusb_context *ctx;
    libusb_device_handle *handle;
    struct libusb_transfer *tx_transfer;
    struct libusb_transfer *rx_transfer;
    uint8_t rxdata[1024];
};

static int open_serial_port(void);
static void close_serial_port(void);
static int poll_serial_port(void);
static int write_serial_port(uint8_t *buffer, int len);

static int read_serial_port(int len);
static void libusb_transfer_read_cb(struct libusb_transfer *transfer);
static void libusb_transfer_write_cb(struct libusb_transfer *transfer);

static struct serial_port port;

extern int parse_ublox_message(uint8_t *data, int len);

struct device device_usb = {
    .open = open_serial_port,
    .close = close_serial_port,
    .poll = poll_serial_port,
    .write = write_serial_port
};

static int open_serial_port(void)
{
    memset(&port, 0, sizeof(port));

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
            uint8_t eps[2] = {LEA6T_USB_EP_TXD, LEA6T_USB_EP_RXD};
            printf("claim interface successfully\n");
            if (2 != libusb_alloc_streams(port.handle, 2, eps, 2)) {
                return -1;
            }
            port.tx_transfer = libusb_alloc_transfer(0);
            port.rx_transfer = libusb_alloc_transfer(0);
            if (!port.tx_transfer || !port.rx_transfer) {
                return -1;
            }
            return read_serial_port(1024);
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
    if (port.tx_transfer) {
        libusb_free_transfer(port.tx_transfer);
    }
    if (port.rx_transfer) {
        libusb_free_transfer(port.rx_transfer);
    }
    if (port.handle) {
        libusb_release_interface(port.handle, LEA6T_USB_SERIAL_IF);
        libusb_close(port.handle);
    }
    if (port.ctx) {
        libusb_exit(port.ctx);
    }
}

static int read_serial_port(int len)
{
    libusb_fill_bulk_stream_transfer(port.rx_transfer, port.handle, LEA6T_USB_EP_RXD,
        2, port.rxdata + 1024 - len, len, libusb_transfer_read_cb, NULL, 0);
    return libusb_submit_transfer(port.rx_transfer);
}

static int write_serial_port(uint8_t *buffer, int len)
{
    uint8_t *data= (uint8_t *)malloc(len);
    if (data) {
        libusb_fill_bulk_stream_transfer(port.tx_transfer, port.handle, LEA6T_USB_EP_TXD,
            1, data, len, libusb_transfer_write_cb, NULL, 0);
        return libusb_submit_transfer(port.tx_transfer);
    }
    else {
        return -1;
    }
}

static int poll_serial_port(void)
{
    int ret = -1;
    const struct libusb_pollfd **list = libusb_get_pollfds(port.ctx);
    if (list) {
        int i;
        int count = 0;
        for (i = 0; list[i] != NULL; i++) {
            count++;
        }
        if (count) {
            struct pollfd *polls = (struct pollfd *)calloc(count, sizeof(*polls));
            for (i = 0; i < count; i++) {
                polls->fd = list[i]->fd;
                polls->events = list[i]->events;
            }
            ret = poll(polls, count, 0);
            if (ret > 0) {
                struct timeval zero = {0, 0};
                ret = libusb_handle_events_timeout(port.ctx, &zero);
            }
            free(polls);
        }
    }

    return ret;
}

static void libusb_transfer_read_cb(struct libusb_transfer *transfer)
{
    if ((LIBUSB_TRANSFER_COMPLETED == transfer->status)
        || (LIBUSB_TRANSFER_OVERFLOW == transfer->status)) {
        int parsed_len = parse_ublox_message(port.rxdata, transfer->actual_length);
        memmove(port.rxdata, port.rxdata + parsed_len, parsed_len);
        read_serial_port(1024 - (transfer->actual_length - parsed_len));
    }
    else {
        read_serial_port(1024);
    }
}

static void libusb_transfer_write_cb(struct libusb_transfer *transfer)
{
    free(transfer->buffer);
}
