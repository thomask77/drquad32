#include "led_ws2812.h"
#include "util.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "dma_io_driver.h"
#include "ws2812_tab.inc"


void led_ws2812_init(void)
{

}


// -------------------- Shell commands --------------------
//
#if 0

#include "command.h"
#include "syscalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "bldc_task.h"

static uint32_t bitmap[100];

int ws2812_brightness = 255;

static void cmd_dma_io_test(void)
{
    TickType_t prev_time = xTaskGetTickCount();

    for(;;) {
        float t = xTaskGetTickCount() * M_TWOPI / 1000;

        for (int i=0; i < ARRAY_SIZE(bitmap); i++) {
            float x = i * M_TWOPI / ARRAY_SIZE(bitmap);

            float r = sinf(x + t * 0.65424) * ws2812_brightness;
            float g = sinf(x + t * 0.43232) * ws2812_brightness;
            float b = sinf(x + t * 0.67652) * ws2812_brightness;

            int ri = clamp(r, 0, 255);
            int gi = clamp(g, 0, 255);
            int bi = clamp(b, 0, 255);

            bitmap[i] = (ri << 16) | (gi << 8) | bi;
        }

        memset(dma_io_tx_buf, 0, sizeof(dma_io_tx_buf));

        dma_io_set_ws2812(dma_io_tx_buf, DMA_IO_TX_SIZE, 0x80, bitmap, ARRAY_SIZE(bitmap));

        // 500 .. 2000 us pulse
        //
        dma_io_set_servo(dma_io_tx_buf, DMA_IO_TX_SIZE, 0x01, 1500e-6 );
        dma_io_set_servo(dma_io_tx_buf, DMA_IO_TX_SIZE, 0x02, sinf(t/10) * 750e-6 + 1500e-6 );

        send_buffer();

        for (int i=0; i<8; i++)
            printf("%4.0f ", dma_io_servo_in[i].t_pulse * 1e6);

        printf("  %lu, %lu us", dma_io_irq_count, dma_io_irq_time);
        printf("\n");

        vTaskDelayUntil(&prev_time, 20);
        if (stdin_chars_avail())
            break;
    }
}

SHELL_CMD(dma_io_test, (cmdfunc_t)cmd_dma_io_test, "Test DMA I/O")


#include "led_ws2812.h"

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


#endif
