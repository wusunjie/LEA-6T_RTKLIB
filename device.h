#ifndef _LEA6T_RTK_H
#define _LEA6T_RTK_H

#include <stdint.h>

struct device {
    int (*open)(void);
    void (*close)(void);
    int (*poll)(void);
    int (*write)(uint8_t *buffer, int len);
};

#endif