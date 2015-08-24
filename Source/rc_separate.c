#include "rc_separate.h"
#include "rc_input.h"
#include "dma_io_driver.h"
#include "util.h"
#include "Shared/ringbuf.h"

#include <stdio.h>

#define MIN_WIDTH   (1500 - 750)
#define MAX_WIDTH   (1500 + 750)


int    rc_ppm_separate_channels = 0x7F;


static struct {
    uint32_t    t0, t;
} channels[8];


static void edge(uint32_t t, int ch, int rising)
{
    // TODO: filter edges
    // TODO: detect timeouts
    //
    if (rising)
        channels[ch].t0 = t;
    else
        channels[ch].t  = t - channels[ch].t0;
}


static void decode(const uint32_t *buf, int len)
{
    static uint32_t t, last_state;
    const uint32_t *src = buf;

    uint32_t  mask32 = rc_ppm_separate_channels;
    mask32 = (mask32<<24) | (mask32<<16) | (mask32<<8) | mask32;

    for (int i=0; i < len; i++) {
        uint32_t state = *src++ & mask32;
        uint32_t diff  = last_state ^ state;

        if (diff) {
            for (int j=0; j<32; j++) {
                if (diff & (1UL << j))
                    edge(t + (j>>3), j & 7, state & (1UL << j));
            }
        }

        last_state = state;
        t += 4;
    }
}


void rc_ppm_separate_update(void)
{
    const uint32_t *ptr1, *ptr2;
    size_t len1, len2;

    dma_io_rx_get_pointers(
        (const void**) &ptr1, &len1,
        (const void**) &ptr2, &len2
    );

    // decode 4 bytes at a time ..
    //
    decode(ptr1, len1 / 4);
    decode(ptr2, len2 / 4);

    // .. and keep alignment
    //
    dma_io_rx_commit((len1 + len2) / 4 * 4);

    rc_input.valid = 1;
    rc_input.rssi  = 100;

    for (int i=0; i<8; i++) {
        int pulse = DMA_TICKS_TO_US(channels[i].t);

        if (rc_ppm_separate_channels & (1<<i)) {
            if (pulse < MIN_WIDTH || pulse > MAX_WIDTH)
                rc_input.valid = 0;

            rc_input.channels[i].pulse = pulse;
        }
        else {
            rc_input.channels[i].pulse = 0;
        }
    }

}


void rc_ppm_separate_init(void)
{
}
