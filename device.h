#ifndef _LEA6T_RTK_H
#define _LEA6T_RTK_H

struct device {
    int (*open)(void);
    void (*close)(void);
    int (*read)(uint8_t *buffer, int len);
    int (*write)(uint8_t *buffer, int len);
};

#endif