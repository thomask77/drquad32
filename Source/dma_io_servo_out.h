#pragma once

#include <stdint.h>

void dma_io_set_servo(
    void *dma_buf, unsigned int dma_len, uint8_t mask,
    int t_pulse
);
