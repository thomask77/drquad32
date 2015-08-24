#include "dma_io_driver.h"
#include "util.h"
#include "Shared/ringbuf.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

// DMA bursts may not cross 1kb boundaries. Make sure that the
// buffers are 16 byte aligned and are a multiple of 16 bytes.
//
// See Reference Manual - "9.3.11 Single and burst transfers"
//
uint8_t  rx_dma_mem[DMA_IO_RX_SIZE] __attribute__ ((aligned(16)));
uint8_t  tx_dma_mem[DMA_IO_TX_SIZE] __attribute__ ((aligned(16)));

STATIC_ASSERT((unsigned)rx_dma_mem % 16 == 0 && sizeof(rx_dma_mem) % 16 == 0);
STATIC_ASSERT((unsigned)tx_dma_mem % 16 == 0 && sizeof(tx_dma_mem) % 16 == 0);

static struct ringbuf rx_buf = { rx_dma_mem, sizeof(rx_dma_mem) };
static struct ringbuf tx_buf = { tx_dma_mem, sizeof(tx_dma_mem) };


static uint8_t  dma_tx_ws2812_bits = 0x80;


size_t dma_io_rx_get_pointers(
    const void **ptr1, size_t *len1,
    const void **ptr2, size_t *len2
)
{
    // Update write position from DMA hardware
    //
    rx_buf.write_pos = rx_buf.buf_size - DMA2_Stream7->NDTR;

    return rb_get_pointers(
        &rx_buf, RB_READ, SIZE_MAX,
        (void **)ptr1, len1, (void **)ptr2, len2
    );
}


size_t dma_io_rx_commit(size_t len)
{
    return rb_commit(&rx_buf, RB_READ, len);
}


void dma_io_tx_start(void)
{
    // Disable DMA requests and restart DMA
    //
    TIM8->DIER &= ~(TIM_DIER_CC1DE | TIM_DIER_CC2DE | TIM_DIER_CC3DE);

    DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);
    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
    DMA_ClearFlag(DMA2_Stream4, DMA_FLAG_TCIF4);
    DMA_Cmd(DMA2_Stream2, ENABLE);
    DMA_Cmd(DMA2_Stream3, ENABLE);
    DMA_Cmd(DMA2_Stream4, ENABLE);

    // Busy wait for the timer to count beyond the last
    // compare event (takes <= 1.25 us)
    //
    __disable_irq();

    while (TIM8->CNT > TIM8->CCR3);
    while (TIM8->CNT < TIM8->CCR3);

    // Initial pin state
    // * Set/Clear bits to match the first tx_buf entry
    // * Pins in WS2812 mode must start with a low level
    //
    GPIOE->BSRRH =  tx_dma_mem[0] |  dma_tx_ws2812_bits;
    GPIOE->BSRRL = ~tx_dma_mem[0] & ~dma_tx_ws2812_bits;

    // Enable DMA requests
    //
    TIM8->DIER |= TIM_DIER_CC1DE | TIM_DIER_CC2DE | TIM_DIER_CC3DE;

    __enable_irq();
}


void dma_io_init(void)
{
    // Initialize peripherals
    //
    RCC->APB2ENR |= RCC_APB2Periph_TIM8;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_GPIOEEN;

    TIM_DeInit(TIM8);
    DMA_DeInit(DMA2_Stream2);
    DMA_DeInit(DMA2_Stream3);
    DMA_DeInit(DMA2_Stream4);
    DMA_DeInit(DMA2_Stream7);

    // Initialize I/O pins as open-drain
    //
    GPIOE->ODR = ~dma_tx_ws2812_bits;

    GPIO_Init(GPIOE, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = 0xFF,
        .GPIO_Mode  = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Low_Speed,
        .GPIO_OType = GPIO_OType_OD,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });

    // Set up DMA channels for PPM / WS2812 output
    //
    // DMA2_Stream2: Ch7 / TIM8_CH1: ws2812_bits -> BSSRL
    // DMA2_Stream3: Ch7 / TIM8_CH2: tx_dma_buf  -> BSSRH
    // DMA2_Stream4: Ch7 / TIM8_CH3: ws2812_bits -> BSSRH
    //
    DMA_Init(DMA2_Stream2, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_7,
        .DMA_PeripheralBaseAddr = (uint32_t)&GPIOE->BSRRL,
        .DMA_Memory0BaseAddr    = (uint32_t)&dma_tx_ws2812_bits,
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = tx_buf.buf_size,
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_High
    });

    // Direct writes to the output register would be nice, but there seems
    // to be a problem with runt pulses when doing DMA transfers to GPIOE->ODR.
    //
    // See  Docs/2014-04-04_DMA_GPIO_Runt_Pulses.png
    //
    // Fortunately, we can use BSRRH as a work-around.
    //
    DMA_Init(DMA2_Stream3, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_7,
        .DMA_PeripheralBaseAddr = (uint32_t)&GPIOE->BSRRH,
        .DMA_Memory0BaseAddr    = (uint32_t)tx_buf.buf,
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = tx_buf.buf_size,
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_High,

        .DMA_FIFOMode           = DMA_FIFOMode_Enable,
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
        .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single
    });

    DMA_Init(DMA2_Stream4, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_7,
        .DMA_PeripheralBaseAddr = (uint32_t)&GPIOE->BSRRH,
        .DMA_Memory0BaseAddr    = (uint32_t)&dma_tx_ws2812_bits,
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = tx_buf.buf_size,
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_High
    });

    // Set up DMA channel for RC_PPM input
    //
    // DMA2_Stream7: Ch7 / TIM8_CH4: IDR -> rx_dma_buf
    //
    DMA_Init(DMA2_Stream7, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_7,
        .DMA_PeripheralBaseAddr = (uint32_t)&GPIOE->IDR,
        .DMA_Memory0BaseAddr    = (uint32_t)rx_buf.buf,
        .DMA_DIR                = DMA_DIR_PeripheralToMemory,
        .DMA_BufferSize         = rx_buf.buf_size,          // /4 ??
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
        .DMA_Mode               = DMA_Mode_Circular,
        .DMA_Priority           = DMA_Priority_High,

        .DMA_FIFOMode           = DMA_FIFOMode_Enable,
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
        .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single
    });

    DMA_Cmd(DMA2_Stream7, ENABLE);

    // Set up TIM8 to trigger the DMA channels
    //
    TIM_TimeBaseInit(TIM8, &(TIM_TimeBaseInitTypeDef) {
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period =  SystemCoreClock / DMA_IO_FREQ - 1,
    } );

    TIM_OCInitTypeDef oc_init = {
        .TIM_OCMode = TIM_OCMode_Timing,
    };

    // Bit timing for WS2812 output
    //
    oc_init.TIM_Pulse = 1;
    TIM_OC1Init(TIM8, &oc_init);

    oc_init.TIM_Pulse = 1 * TIM8->ARR / 3;
    TIM_OC2Init(TIM8, &oc_init);

    oc_init.TIM_Pulse = 2 * TIM8->ARR / 3;
    TIM_OC3Init(TIM8, &oc_init);

    // Generate DMA requests for PWM input
    //
    oc_init.TIM_Pulse = 2.5 * TIM8->ARR / 3;
    TIM_OC4Init(TIM8, &oc_init);

    TIM_DMACmd(TIM8, TIM_DMA_CC4, ENABLE);


    TIM_Cmd(TIM8, ENABLE);
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

#endif
