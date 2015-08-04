#include "term_cobs.h"
#include "util.h"
#include "Shared/errors.h"
#include "Shared/ringbuf.h"
#include "Shared/cobsr.h"
#include "Shared/crc16.h"
#include "Shared/msg_structs.h"

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>


#define XBEE_BAUDRATE  460800
#define RX_QUEUE_SIZE  1024


struct cobs_stats {
    uint32_t    rx_bytes;
    uint32_t    rx_packets;
    uint32_t    rx_crc_errors;
    uint32_t    rx_overrun;

    uint32_t    tx_bytes;
    uint32_t    tx_packets;
};

static struct cobs_stats cobs_stats;

static struct ringbuf rx_dma_buf = RINGBUF(1024);
static struct ringbuf tx_dma_buf = RINGBUF(1024);

size_t tx_dma_len;


static SemaphoreHandle_t  rx_sem;
static SemaphoreHandle_t  tx_sem;

static QueueHandle_t rx_queue;


void start_next_tx(void)
{
    void    *ptr1;
    size_t  len1;

    rb_get_pointers(&tx_dma_buf, RB_READ, SIZE_MAX, &ptr1, &len1, NULL, NULL);

    if (len1 > 0) {
        tx_dma_len = len1;
        DMA1_Stream3->M0AR = (uint32_t)ptr1;
        DMA1_Stream3->NDTR = tx_dma_len;
        DMA1_Stream3->CR |= DMA_SxCR_EN;
    }
}


// DMA TX Interrupt (transfer complete)
//
void DMA1_Stream3_IRQHandler(void)
{
    cobs_stats.tx_bytes += tx_dma_len;
    cobs_stats.tx_packets++;

    rb_commit(&tx_dma_buf, RB_READ, tx_dma_len);

    // Clear all flags
    //
    DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3  |
                  DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
                  DMA_LIFCR_CFEIF3;

    // Start next transfer (if any)
    //
    start_next_tx();

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(tx_sem, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


void dma_send(void *data, int len)
{
    // TODO: This must be atomic.
    // Messages may not be split if
    // several tasks call dma_send.
    // Also combine with cobs_encode.
    //
    while (len > 0) {
        int chunk = rb_write(&tx_dma_buf, data, len);

        if (chunk != 0) {
            data += chunk;
            len  -= chunk;

            if (!(DMA1_Stream3->CR & DMA_SxCR_EN))
                start_next_tx();
        }
        else {
            xSemaphoreTake(tx_sem, portMAX_DELAY);
        }
    }
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
    static uint8_t tx_packet_buf[1024];

    msg.h.id = MSG_ID_SHELL_TO_PC;

    if (len > sizeof(msg.data))
        len = sizeof(msg.data);

    msg.h.data_len = len;
    memcpy(msg.data, ptr, len);

    msg.h.crc = msg_calc_crc(&msg.h);

    // Encode and write to ringbuf
    //
    int encoded_len = cobsr_encode(
        tx_packet_buf, sizeof(tx_packet_buf) - 1,   // 1 byte for end-of-packet
        &msg.h.crc, 2 + 2 + msg.h.data_len          // +CRC +ID
    );
    tx_packet_buf[encoded_len++] = 0;

    dma_send(tx_packet_buf, encoded_len);

    return len;
}


static ssize_t term_cobs_read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    // Blocking wait for the first char
    //
    unsigned timeout = portMAX_DELAY;

    char *dest = ptr;
    int  received = 0;
    static char last_c;

    while (len--) {
        char c;
        if (!xQueueReceive(rx_queue, &c, timeout))
            break;

        // Convert terminal CRLF to c-newline
        //
        if (c == '\n' && last_c == '\r')
            if (!xQueueReceive(rx_queue, &c, timeout))
                break;

        if (c == '\r')
            *dest++ = '\n';
        else
            *dest++ = c;

        last_c  = c;
        timeout = 0;
        received++;
    }

    return received;
}


static ssize_t term_cobs_chars_avail_r(struct _reent *r, int fd)
{
    return uxQueueMessagesWaiting(rx_queue);
}


static uint8_t   len,  last_len;

static uint8_t   rx_packet_buf[1024];
static ssize_t   rx_packet_len;

struct  msg_generic   rx_packet;


static void emit(uint8_t c)
{
    if (rx_packet_len >= sizeof(rx_packet_buf))
        rx_packet_len = -1;

    if (rx_packet_len == -1)
        cobs_stats.rx_overrun++;
    else
        rx_packet_buf[rx_packet_len++] = c;
}


static int decode_char(uint8_t c)
{
    if (c == 0) {
        if (len > 1)
            emit(last_len);
        len = 0;
        return 1;
    }

    if (len == 0) {
        last_len = c;
        len = c;
        return 0;
    }

    if (--len) {
        emit(c);
        return 0;
    }
    else {
        emit(0);
        return decode_char(c);
    }
}


int cobs_recv(void)
{
    // Update write position from DMA hardware
    //
    rx_dma_buf.write_pos = rx_dma_buf.buf_size - DMA1_Stream1->NDTR;

    void    *ptr;
    size_t  len;

    rb_get_pointers(
        &rx_dma_buf, RB_READ, SIZE_MAX, &ptr, &len, NULL, NULL
    );

    // TODO: Wenn der Buffer schon eine len2 hat, direkt bearbeiten!!
    //
    for (int i=0; i<len; i++) {
        if (decode_char( ((char*)ptr)[i] )) {
            int ret = rx_packet_len;
            rx_packet_len = 0;

            if (ret < 0) {
                errno = ECOBSR_DECODE_OUT_BUFFER_OVERFLOW;
            }
            else if (ret < 4) {
                errno = EMSG_TOO_SHORT;
            }
            else {
                rx_packet.h.data_len = ret - 2 - 2;
                memcpy(&rx_packet.h.crc, rx_packet_buf, ret);

                if (msg_calc_crc(&rx_packet.h) != rx_packet.h.crc) {
                    ret = -1;
                    errno = EMSG_CRC;
                }
            }

            rb_commit(&rx_dma_buf, RB_READ, i+1);
            return ret;
        }
    }

    rb_commit(&rx_dma_buf, RB_READ, len);
    return 0;
}


void handle_shell_from_pc(const struct msg_shell_from_pc *msg)
{
    for (int i=0; i<msg->h.data_len; i++)
        xQueueSend(rx_queue, &msg->data[i], 0);
}


void handle_unknown_message(const struct msg_generic *msg)
{
    printf("unknown message(0x%04x, %d)\n", msg->h.id, msg->h.data_len);
    hexdump(&msg->h.data, msg->h.data_len);
}


void handle_messsage(struct msg_generic *msg)
{
    switch (msg->h.id) {
    case MSG_ID_SHELL_FROM_PC:  handle_shell_from_pc((void*)msg);      break;
    default:                    handle_unknown_message((void*)msg);    break;
    }
}


void cobs_task(void *pv)
{
    for(;;) {

        for(;;) {
            ssize_t len = cobs_recv();

            if (len == 0) {
                break;
            }
            else if (len < 0) {
                printf("recv failed: %s\n", strerror(errno));
            }
            else {
                handle_messsage(&rx_packet);
            }
        }

        vTaskDelay(1);
    }
}


void term_cobs_init(void)
{
    if (!rx_sem)    rx_sem = xSemaphoreCreateBinary();
    if (!tx_sem)    tx_sem = xSemaphoreCreateBinary();
    if (!rx_queue)  rx_queue = xQueueCreate(RX_QUEUE_SIZE, 1);

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
        .DMA_Memory0BaseAddr    = (uint32_t)rx_dma_buf.buf,
        .DMA_DIR                = DMA_DIR_PeripheralToMemory,
        .DMA_BufferSize         = rx_dma_buf.buf_size,
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
        .DMA_Memory0BaseAddr    = (uint32_t)tx_dma_buf.buf,
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = tx_dma_buf.buf_size,
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_Low,
    } );

    // Initialize IRQs
    //
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

    printf("Starting COBS task..\n");
    xTaskCreate(cobs_task, "cobs_task", 256, NULL, 0, NULL);
    vTaskDelay(100);
}


struct  file_ops   term_cobs_ops = {
    .read_r        = term_cobs_read_r,
    .write_r       = term_cobs_write_r,
    .chars_avail_r = term_cobs_chars_avail_r
};
