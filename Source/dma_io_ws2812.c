#include "dma_io_ws2812.h"
#include "dma_io_driver.h"

#include "ws2812_tab.inc"

void dma_io_set_ws2812(
    void *dma_buf, int dma_len, uint8_t mask,
    uint32_t *bitmap, int width
)
{
    uint32_t *dst = dma_buf;
    uint32_t  mask32 = (mask<<24) | (mask<<16) | (mask<<8) | mask;

    // TODO: Clipping

    for (int i=0; i < width; i++) {
        const uint32_t rgb = bitmap[i];
        *dst++ |= ws2812_tab[(rgb >>  8) & 255][0] & mask32;
        *dst++ |= ws2812_tab[(rgb >>  8) & 255][1] & mask32;
        *dst++ |= ws2812_tab[(rgb >> 16) & 255][0] & mask32;
        *dst++ |= ws2812_tab[(rgb >> 16) & 255][1] & mask32;
        *dst++ |= ws2812_tab[(rgb >>  0) & 255][0] & mask32;
        *dst++ |= ws2812_tab[(rgb >>  0) & 255][1] & mask32;
    }
}

