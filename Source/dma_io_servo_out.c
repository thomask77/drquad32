#include "dma_io_servo_out.h"
#include "dma_io_driver.h"
#include "util.h"


void dma_io_set_servo(
    void *dma_buf, unsigned int dma_len, uint8_t mask,
    int t_pulse
)
{
    uint32_t index = DMA_US_TO_TICKS( t_pulse );

    if (index < 0 || index >= dma_len)
        return;

    // Initially set to high, clear after pulse time
    //
    ((uint8_t*)dma_buf)[index] |= mask;
}


