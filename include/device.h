#ifndef _LEA6T_RTK_H
#define _LEA6T_RTK_H

#include <stdint.h>

#include <poll.h>

struct device {
    int (*open)(void);
    void (*close)(void);
    int (*get_pollfd)(struct pollfd **polls);
    int (*handle_event)(void);
    int (*write)(uint8_t *buffer, int len);
};

#endif