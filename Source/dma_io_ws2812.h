#pragma once

#include <stdint.h>

void dma_io_set_ws2812(
    void *dma_buf, int dma_len, uint8_t mask,
    uint32_t *bitmap, int width
);
