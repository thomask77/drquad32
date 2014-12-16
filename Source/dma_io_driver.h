#pragma once

#include <stdint.h>

#define DMA_IO_FREQ     800000

#define DMA_TICKS_TO_US(x)  (10 * (x) /  8)
#define DMA_US_TO_TICKS(x)  (8  * (x) / 10)

// Receive buffer for PPM signal decoding.
// 2ms buffer will cause a 1 kHz interrupt rate.
//
#define DMA_IO_RX_SIZE  (2 * DMA_IO_FREQ / 1000)


// Transmit buffer for PPM pulses and WS2812 bit stream.
// Maximum PPM pulse length of 3ms or 100 RGB LEDs.
//
#define DMA_IO_TX_SIZE  (3 * DMA_IO_FREQ / 1000)

extern uint8_t  dma_tx_buf[DMA_IO_TX_SIZE];

extern int ws2812_brightness;

extern volatile uint32_t dma_io_irq_count;
extern volatile uint32_t dma_io_irq_time;

void dma_io_clear(void);
void dma_io_send(void);

void dma_io_init(void);
