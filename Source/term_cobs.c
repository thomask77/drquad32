#include "term_cobs.h"
#include "Shared/ringbuf.h"
#include "Shared/cobsr.h"
#include "Shared/crc16.h"
#include "Shared/msg_structs.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>

#define XBEE_BAUDRATE  460800

struct cobs_stats {
    uint32_t    rx_bytes;
    uint32_t    rx_packets;
    uint32_t    rx_crc_errors;

    uint32_t    tx_bytes;
    uint32_t    tx_packets;
};

static uint8_t tx_dma_buf[1024];    // muss nur für ein paket reichen
static uint8_t rx_dma_buf[1024];    // mindestens newlib-BUFSIZ

static size_t  tx_dma_len;

static SemaphoreHandle_t  rx_sem;
static SemaphoreHandle_t  tx_sem;

static volatile struct cobs_stats cobs_stats;


/**
 * TODO
 *
 * void USART3_IRQHandler(void)
 * {
 *   // Dummy reads to clear ORE flag in case of an overrun
 *   // (see STM32 UART register documentation)
 *   //
 *   if (USART3->SR & USART_SR_ORE)
 *       cobs_stats.rx_overrun++;
 * }
 */


// DMA TX Interrupt (transfer complete)
//
void DMA1_Stream3_IRQHandler(void)
{
    cobs_stats.tx_bytes += tx_dma_len;
    cobs_stats.tx_packets++;

    // Clear all flags
    //
    DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3  |
                  DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
                  DMA_LIFCR_CFEIF3;

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(tx_sem, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


static crc16_t msg_calc_crc(const struct msg_header *msg)
{
    crc16_t crc = crc16_init();
    crc = crc16_update(crc, (uint8_t*)&msg->id, 2 + msg->data_len);
    crc = crc16_finalize(crc);
    return crc;
}


static ssize_t term_cobs_write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    // Wait for previous transfer to finish
    //
    xSemaphoreTake(tx_sem, portMAX_DELAY);

    // Encode message
    //
    static struct msg_shell_to_pc msg;

    msg.h.id = MSG_ID_SHELL_TO_PC;

    if (len > sizeof(msg.data))
        len = sizeof(msg.data);

    msg.h.data_len = len;
    memcpy(msg.data, ptr, len);

    msg.h.crc = msg_calc_crc(&msg.h);

    // Encode to txbuf and  add end-of-packet marker
    //
    tx_dma_len = cobsr_encode(
        tx_dma_buf, sizeof(tx_dma_buf) - 1,   // 1 byte for end-of-packet
        &msg.h.crc, 2 + 2 + msg.h.data_len    // +CRC +ID
    );

    tx_dma_buf[tx_dma_len++] = 0;

    // Start transfer
    //
    DMA1_Stream3->NDTR = tx_dma_len;
    DMA1_Stream3->CR |= DMA_SxCR_EN;

    return len;
}


static ssize_t term_cobs_read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    // TODO
    // - Semantik wie bei UDP-recv():
    //   Nicht abgeholte Bytes in einem Paket sind verloren
    //
    // for(;;) {
    //   Read DMA1_Stream3->NDTR;
    //   if (buf[i] == 0)
    //       break;
    //   vTaskDelay(1);
    // }
    //
    // Bytes in temp-buffer kopieren
    // COBS nach ptr decoden
    //
    // später: temp-buffer wegmachen (knifflig wegen wrap-around)
    //
    return 0;
}


static ssize_t term_cobs_chars_avail_r(struct _reent *r, int fd)
{
//    return uxQueueMessagesWaiting(rx_queue);
    return 0;
}


void term_cobs_init(void)
{
    if (!rx_sem)  rx_sem = xSemaphoreCreateBinary();
    if (!tx_sem)  tx_sem = xSemaphoreCreateBinary();

    xSemaphoreGive(tx_sem);

    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    // Initialize USART
    //
    USART_DeInit(USART3);

    USART_Init(USART3, &(USART_InitTypeDef) {
        .USART_BaudRate = XBEE_BAUDRATE,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_1,
        .USART_Parity = USART_Parity_No,
        .USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS,
        .USART_Mode = USART_Mode_Rx | USART_Mode_Tx
    });

    // Initialize GPIO pins
    //
    // PD8  USART3_TX   XBEE_DIN
    // PD9  USART3_RX   XBEE_DOUT
    // PB13 USART3_CTS
    // PB14 USART3_RTS
    //
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_USART3);

    GPIO_Init(GPIOD, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9,
        .GPIO_Speed = GPIO_Speed_25MHz,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });

    GPIO_Init(GPIOB, &(GPIO_InitTypeDef) {
        .GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14,
        .GPIO_Speed = GPIO_Speed_25MHz,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_DOWN
    });

    // Initialize DMA channels
    //
    // DMA1_Stream1 Channel 4   USART3_RX   (circular, polled)
    // DMA1_Stream3 Channel 4   USART3_TX   (single shot + irq)
    //
    DMA_Init(DMA1_Stream1, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_4,
        .DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR,
        .DMA_Memory0BaseAddr    = (uint32_t)&rx_dma_buf,
        .DMA_DIR                = DMA_DIR_PeripheralToMemory,
        .DMA_BufferSize         = sizeof(rx_dma_buf),
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
        .DMA_Mode               = DMA_Mode_Circular,
        .DMA_Priority           = DMA_Priority_Low,
    } );

    DMA_Init(DMA1_Stream3, &(DMA_InitTypeDef) {
        .DMA_Channel            = DMA_Channel_4,
        .DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR,
        .DMA_Memory0BaseAddr    = (uint32_t)&tx_dma_buf,
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = sizeof(tx_dma_buf),
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_Low,
    } );

    // Initialize IRQs
    //
//    NVIC_Init(&(NVIC_InitTypeDef) {
//        .NVIC_IRQChannel = USART3_IRQn,
//        .NVIC_IRQChannelPreemptionPriority =
//                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
//        .NVIC_IRQChannelSubPriority = 0,
//        .NVIC_IRQChannelCmd = ENABLE
//    });

    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = DMA1_Stream3_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });

    // Start peripherals
    //
    DMA_Cmd(DMA1_Stream1, ENABLE);

    USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
    USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);

    DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);

    USART_Cmd(USART3, ENABLE);
}


struct  file_ops   term_cobs_ops = {
    .read_r        = term_cobs_read_r,
    .write_r       = term_cobs_write_r,
    .chars_avail_r = term_cobs_chars_avail_r
};
