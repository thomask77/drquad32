#include "dma_io_servo_in.h"
#include "dma_io_driver.h"
#include "rc_input.h"
#include "util.h"


struct dma_io_servo_in dma_io_servo_in[8];


static void edge(uint32_t t, int ch, int rising)
{
    if (rising) {
        dma_io_servo_in[ch].t_rising = t;
    }
    else {
        dma_io_servo_in[ch].t_pulse = DMA_TICKS_TO_US(
            t - dma_io_servo_in[ch].t_rising
        );
    }
}


void dma_io_decode_servo(const void *dma_buf, unsigned int dma_len, uint8_t mask)
{
    // TODO: filter edges
    // TODO: detect timeouts
    //

    static uint32_t t, last_state;

    const uint32_t *src = dma_buf;
    uint32_t  mask32 = (mask<<24) | (mask<<16) | (mask<<8) | mask;

    for (unsigned int i=0; i < dma_len / 4; i++) {
        uint32_t state = *src++ & mask32;
        uint32_t diff  = last_state ^ state;

        if (diff) {
            for (unsigned int j=0; j<32; j++) {
                if (diff & (1UL << j))
                    edge(t + (j>>3), j & 7, state & (1UL << j));
            }
        }

        last_state = state;
        t += 4;
    }
}
