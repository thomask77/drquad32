#pragma once

#include <stdint.h>

struct dma_io_servo_in {
    uint32_t    t_rising;
    int         t_pulse;
};


extern struct dma_io_servo_in dma_io_servo_in[8];


void dma_io_decode_servo(
    const void *dma_buf, unsigned int dma_len, uint8_t mask

);
