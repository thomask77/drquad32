#include "term_xbee.h"
#include "command.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

#define XBEE_BAUDRATE  115200

#define RX_QUEUE_SIZE  1024
#define TX_QUEUE_SIZE  1024

static volatile struct xbee_stats {
    uint32_t    rx_bytes;
    uint32_t    tx_bytes;
    uint32_t    rx_overrun;
    uint32_t    tx_overrun;
} xbee_stats;


static QueueHandle_t rx_queue = NULL;
static QueueHandle_t tx_queue = NULL;


int rts_hack;


void EXTI15_10_IRQHandler(void)
{
    if (GPIOB->IDR & GPIO_Pin_14) {
        rts_hack = 1;
        USART3->CR1 &= ~USART_CR1_TXEIE;
    }
    else {
        rts_hack = 0;
        USART3->CR1 |= USART_CR1_TXEIE;
    }

    EXTI->PR = EXTI_Line14;
    EXTI->PR;   // dummy read to prevent glitches
}



void USART3_IRQHandler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if (USART3->SR & USART_SR_RXNE) {
        char  c = USART3->DR;

        if (xQueueSendFromISR(rx_queue, &c, &xHigherPriorityTaskWoken))
            xbee_stats.rx_bytes++;
        else
            xbee_stats.rx_overrun++;
    }

    if (USART3->SR & USART_SR_TXE) {
        char c;
        if (xQueueReceiveFromISR(tx_queue, &c, &xHigherPriorityTaskWoken)) {
            // send a queued byte
            //
            USART3->DR = c;
            xbee_stats.tx_bytes++;
        }
        else {
            // nothing to send, disable interrupt
            //
            USART3->CR1 &= ~USART_CR1_TXEIE;
        }
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


static ssize_t xbee_write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    const char *src = ptr;
    int sent = 0;

    while (len--) {
/*
        // Convert c-newlines to terminal CRLF
        //
        if (*src == '\n') {
            xQueueSend(tx_queue, "\r", portMAX_DELAY);
            USART3->CR1 |= USART_CR1_TXEIE;
        }
*/
        xQueueSend(tx_queue, src++, portMAX_DELAY);
        USART3->CR1 |= USART_CR1_TXEIE;

        sent++;
    }
    return sent;
}


static ssize_t xbee_read_r(struct _reent *r, int fd, void *ptr, size_t len)
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


static ssize_t xbee_chars_avail_r(struct _reent *r, int fd)
{
    return uxQueueMessagesWaiting(rx_queue);
}


static void xbee_init_uart(void)
{
    static int is_initialized;

    if (is_initialized)
        return;

    is_initialized = 1;

    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;
    RCC->APB1ENR |= RCC_APB1Periph_USART3;

    USART_DeInit(USART3);

    // Initialize USART
    //
    USART_Init(USART3, &(USART_InitTypeDef) {
        .USART_BaudRate = XBEE_BAUDRATE,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_1,
        .USART_Parity = USART_Parity_No ,
//        .USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS,  // See HACK above
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

    USART_Cmd(USART3, ENABLE);
}


void xbee_poll_send(const char *ch)
{
    xbee_init_uart();

    while (*ch) {
        if (*ch == '\n') {
            while (!(USART3->SR & USART_FLAG_TXE));
            USART3->DR = '\r';
        }
        while (!(USART3->SR & USART_FLAG_TXE));
        USART3->DR = *ch++;
    }
}


/**
 * Initialize the XBee UART.
 *
 */
void xbee_init(void)
{
    xbee_init_uart();

    if (!rx_queue) rx_queue = xQueueCreate(RX_QUEUE_SIZE, 1);
    if (!tx_queue) tx_queue = xQueueCreate(TX_QUEUE_SIZE, 1);

    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = USART3_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    // Interrupt on RTS change
    //
    RCC->APB2ENR |= RCC_APB2Periph_SYSCFG;
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource14);

    EXTI_Init(&(EXTI_InitTypeDef) {
        .EXTI_Line    = EXTI_Line14,
        .EXTI_Mode    = EXTI_Mode_Interrupt,
        .EXTI_Trigger = EXTI_Trigger_Both,
        .EXTI_LineCmd = ENABLE
    });

    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = EXTI15_10_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
                configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });

    dev_register("xbee", &term_xbee_ops);
}


struct file_ops term_xbee_ops = {
    .read_r        = xbee_read_r,
    .write_r       = xbee_write_r,
    .chars_avail_r = xbee_chars_avail_r
};


// -------------------- Shell commands --------------------
//
static void cmd_xbee_stats(void)
{
    printf("rx_bytes:   %10lu\n", xbee_stats.rx_bytes);
    printf("rx_overrun: %10lu\n", xbee_stats.rx_overrun);
    printf("tx_bytes:   %10lu\n", xbee_stats.tx_bytes);
    printf("tx_overrun: %10lu\n", xbee_stats.tx_overrun);
}

SHELL_CMD(xbee_stats, (cmdfunc_t)cmd_xbee_stats, "show XBee statistics")
