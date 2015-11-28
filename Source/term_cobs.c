#include "term_cobs.h"
#include "util.h"
#include "attitude.h"
#include "sensors.h"

#include "errors.h"
#include "ringbuf.h"
#include "cobsr_codec.h"

#include "crc16.h"
#include "msg_structs.h"

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XBEE_BAUDRATE  460800
#define RX_QUEUE_SIZE  1024

static struct ringbuf rx_dma_buf = RINGBUF(1024);
static struct ringbuf tx_dma_buf = RINGBUF(1024);

static size_t tx_dma_len;

static SemaphoreHandle_t  rx_irq_sem;
static SemaphoreHandle_t  tx_irq_sem;
static SemaphoreHandle_t  tx_buf_mtx;

static QueueHandle_t rx_queue;
struct msg_generic   rx_packet;


/** RINGBUFFER <-> UART/DMA */

static void tx_dma_update(void)
{
    if (DMA1_Stream3->CR & DMA_SxCR_EN) {
        // Transfer still in progress
        //
        return;
    }

    void    *ptr1;
    size_t  len1;

    tx_dma_len = rb_get_pointers(&tx_dma_buf, RB_READ, SIZE_MAX, &ptr1, &len1, NULL, NULL);

    if (tx_dma_len > 0) {
        DMA1_Stream3->M0AR = (uint32_t)ptr1;
        DMA1_Stream3->NDTR = tx_dma_len;
        DMA1_Stream3->CR |= DMA_SxCR_EN;
    }
}


static void rx_dma_update(void)
{
    // Update write position from DMA hardware
    //
    rx_dma_buf.write_pos = rx_dma_buf.buf_size - DMA1_Stream1->NDTR;
}


void DMA1_Stream3_IRQHandler(void)
{
    // Clear all flags
    //
    DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3  |
                  DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 |
                  DMA_LIFCR_CFEIF3;

    // Commit last transfer and start next (if any)
    //
    rb_commit(&tx_dma_buf, RB_READ, tx_dma_len);
    tx_dma_update();

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(tx_irq_sem, &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


/** COBS <-> RINGBUFFER */

static crc16_t msg_calc_crc(const struct msg_header *msg)
{
    crc16_t crc = crc16_init();
    crc = crc16_update(crc, (uint8_t*)&msg->id, 2 + msg->data_len);
    crc = crc16_finalize(crc);
    return crc;
}


static int msg_send(const struct msg_header *msg)
{
    if (!xSemaphoreTake(tx_buf_mtx, 100)) {
        errno = ETIMEDOUT;
        return -1;
    }

    struct cobsr_encoder_state enc = {
        .in      = (char*)&msg->crc,
        .in_end  = (char*)&msg->crc + 2 + 2 + msg->data_len,   // CRC + ID + data
    };

    int complete = 0;
    while (!complete) {
        // TODO do not busy loop!
        //
        void   *ptr1;
        size_t len1;

        rb_get_pointers(&tx_dma_buf, RB_WRITE, SIZE_MAX, &ptr1, &len1, NULL, NULL);

        if (len1) {
            enc.out     = (char*)ptr1;
            enc.out_end = (char*)ptr1 + len1;

            complete = cobsr_encode(&enc);

            // Commit and update DMA
            //
            rb_commit(&tx_dma_buf, RB_WRITE, enc.out - (char*)ptr1);
            tx_dma_update();
        }
    }

    xSemaphoreGive(tx_buf_mtx);
    return 1;
}



static struct cobsr_decoder_state dec;


static void reset_decoder(void)
{
    dec.out     = (char*)&rx_packet.h.crc;
    dec.out_end = (char*)&rx_packet.h.crc + 2 + 2 + sizeof(rx_packet.data);
}


static int msg_recv(void)
{
    void   *ptr1, *ptr2;
    size_t len1, len2;

    rb_get_pointers(&rx_dma_buf, RB_READ, SIZE_MAX, &ptr1, &len1, &ptr2, &len2);

    dec.in = (char*)ptr1;
    dec.in_end = (char*)ptr1 + len1;

    int complete = cobsr_decode(&dec);
    size_t in_len = dec.in - (char*)ptr1;

    if (!complete && len2) {
        dec.in = (char*)ptr2;
        dec.in_end = (char*)ptr2 + len2;
        complete = cobsr_decode(&dec);
        in_len += dec.in - (char*)ptr2;
    }

    rb_commit(&rx_dma_buf, RB_READ, in_len);

    if (!complete)
        return 0;

    size_t out_len = dec.out - (char*)&rx_packet.h.crc;

    reset_decoder();

    if (out_len < 4) {
        errno = EMSG_TOO_SHORT;
        return -1;
    }

    rx_packet.h.data_len = out_len - 2 - 2;

    if (msg_calc_crc(&rx_packet.h) != rx_packet.h.crc) {
        errno = EMSG_CRC;
        return -1;
    }

    return 1;
}


/** TERMINAL messages <-> character device stuff */

static ssize_t term_cobs_write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    // Encode message
    //
    static struct msg_shell_to_pc msg;

    msg.h.id = MSG_ID_SHELL_TO_PC;

    if (len > sizeof(msg.data))
        len = sizeof(msg.data);

    msg.h.data_len = len;
    memcpy(msg.data, ptr, len);

    msg.h.crc = msg_calc_crc(&msg.h);

    msg_send(&msg.h);
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


void term_cobs_init(void)
{
    if (!rx_irq_sem)    rx_irq_sem = xSemaphoreCreateBinary();
    if (!tx_irq_sem)    tx_irq_sem = xSemaphoreCreateBinary();
    if (!tx_buf_mtx)    tx_buf_mtx = xSemaphoreCreateMutex();
    if (!rx_queue)      rx_queue   = xQueueCreate(RX_QUEUE_SIZE, 1);

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
}


struct  file_ops   term_cobs_ops = {
    .read_r        = term_cobs_read_r,
    .write_r       = term_cobs_write_r,
    .chars_avail_r = term_cobs_chars_avail_r
};


/** Debug message handling, periodic send */


static void handle_shell_from_pc(const struct msg_shell_from_pc *msg)
{
    for (int i=0; i<msg->h.data_len; i++)
        xQueueSend(rx_queue, &msg->data[i], 0);
}


static void handle_messsage(const struct msg_generic *msg)
{
    switch (msg->h.id) {
    case MSG_ID_SHELL_FROM_PC:
        handle_shell_from_pc((void*)msg);
        break;

    default:
        printf("unknown message(0x%04x, %d)\n", msg->h.id, msg->h.data_len);
        hexdump(&msg->data, msg->h.data_len);
        break;
    }
}


static void send_imu_data(void)
{
    struct sensor_data d;
    sensor_read(&d);

    struct msg_imu_data msg;
    msg.h.id = MSG_ID_IMU_DATA;
    msg.h.data_len  = MSG_DATA_LENGTH(struct msg_imu_data);

    msg.acc_x       = d.acc.x;
    msg.acc_y       = d.acc.y;
    msg.acc_z       = d.acc.z;
    msg.gyro_x      = d.gyro.x;
    msg.gyro_y      = d.gyro.y;
    msg.gyro_z      = d.gyro.z;
    msg.mag_x       = d.mag.x;
    msg.mag_y       = d.mag.y;
    msg.mag_z       = d.mag.z;
    msg.baro_hpa    = d.pressure;
    msg.baro_temp   = d.baro_temp;

    msg.h.crc = msg_calc_crc(&msg.h);
    msg_send(&msg.h);
}


static void send_dcm_matrix(void)
{
    struct msg_dcm_matrix msg;
    msg.h.id = MSG_ID_DCM_MATRIX;
    msg.h.data_len  = MSG_DATA_LENGTH(struct msg_dcm_matrix);

    msg.m00 = dcm.matrix.m00;
    msg.m01 = dcm.matrix.m01;
    msg.m02 = dcm.matrix.m02;
    msg.m10 = dcm.matrix.m10;
    msg.m11 = dcm.matrix.m11;
    msg.m12 = dcm.matrix.m12;
    msg.m20 = dcm.matrix.m20;
    msg.m21 = dcm.matrix.m21;
    msg.m22 = dcm.matrix.m22;

    msg.h.crc = msg_calc_crc(&msg.h);
    msg_send(&msg.h);
}


static void send_dcm_reference(void)
{
    struct msg_dcm_reference msg;
    msg.h.id = MSG_ID_DCM_REFERENCE;
    msg.h.data_len = MSG_DATA_LENGTH(struct msg_dcm_reference);

    msg.down_x  = dcm.down.x;
    msg.down_y  = dcm.down.y;
    msg.down_z  = dcm.down.z;
    msg.north_x = dcm.north.x;
    msg.north_y = dcm.north.y;
    msg.north_z = dcm.north.z;

    msg.h.crc = msg_calc_crc(&msg.h);
    msg_send(&msg.h);
}


static void send_periodic(void)
{
    static TickType_t   t_last_dcm_reference;
    static TickType_t   t_last_attitude;
    static TickType_t   t_last_imu_data;

    TickType_t  t = xTaskGetTickCount();

    if (t - t_last_attitude > 16) {
        send_dcm_matrix();
        t_last_attitude = t;
    }

    if (t - t_last_dcm_reference > 16) {
        send_dcm_reference();
        t_last_dcm_reference = t;
    }

    if (t - t_last_imu_data > 16) {
        send_imu_data();
        t_last_imu_data = t;
    }
}


static void cobs_task_loop(void)
{
    rx_dma_update();

    for(;;) {
        int len = msg_recv();
        if (len == 0)
            break;

        if (len >= 0)
            handle_messsage(&rx_packet);
        else
            printf("recv failed: %s\n", strerror(errno));
    }

    if (xTaskGetTickCount() > 1000)
        send_periodic();
}


void cobs_task(void *pv)
{
    reset_decoder();

    for(;;) {
        cobs_task_loop();
        vTaskDelay(1);
    }
}

