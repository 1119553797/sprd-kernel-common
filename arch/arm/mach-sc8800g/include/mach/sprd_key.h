#ifndef _SPRD_KPAD_H
#define _SPRD_KPAD_H

struct sprd_kpad_platform_data {
        int rows;
        int cols;
        const unsigned int *keymap;
        unsigned short keymapsize;
        unsigned short repeat;
        u32 debounce_time;      /* in ns */
        u32 coldrive_time;      /* in ns */
        u32 keyup_test_interval; /* in ms */
};

#define KEYVAL(row, col, val) (((row) << 24) | ((col) << 16) | (val))

#endif
