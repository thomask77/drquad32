#pragma once

#include <stdint.h>

struct bmp180_regs {
    struct {
        uint8_t ctrl_meas;  // 0xF4
        uint8_t reserved;   // 0xF5
        uint8_t msb;        // 0xF6
        uint8_t lsb;        // 0xF7
        uint8_t xlsb;       // 0xF8
    } ut, up;
};

struct bmp180_data {
    uint32_t clipflags;
    float temp;             // [°C]
    float pressure;         // [hPa]
};

int bmp180_start_ut(void);
int bmp180_start_up(void);

int bmp180_read(struct bmp180_regs *regs);

int bmp180_convert(struct bmp180_data *data, const struct bmp180_regs *regs);
int bmp180_init(void);
