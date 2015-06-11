#include "uart.h"
#include "stm32f4xx.h"
#include "Shared/ringbuf.h"

static struct ringbuf rx_buf = RINGBUF( 4096 );
static struct ringbuf tx_buf = RINGBUF( 4096 );


void USART3_IRQHandler(void)
{
    uint32_t sr = USART3->SR;

    if (sr & USART_SR_RXNE) {
        rb_putchar(&rx_buf, USART3->DR);
    }

    if (sr & USART_SR_TXE) {
        int c = rb_getchar(&tx_buf);
        if (c >= 0)
            USART3->DR = c;
        else
            USART3->CR1 &= ~USART_CR1_TXEIE;
    }
}


void uart_putc(char c)
{
    int ret = 0;

    do {
        ret = rb_putchar(&tx_buf, c);
        USART3->CR1 |= USART_CR1_TXEIE;
    } while (ret < 0);
}


int uart_getc(void)
{
    return rb_getchar(&rx_buf);
}


void printf_putchar(char c)
{
    // Convert c-newlines to terminal CRLF
    //
    if (c == '\n')
        uart_putc('\r');

    uart_putc(c);
}


void uart_flush(void)
{
    while (rb_bytes_used(&tx_buf) > 0);
    while (!(USART3->SR & USART_SR_TC));
}


ssize_t uart_write(const void *buf, size_t len)
{
    const char *s = buf;
    for (int i=0; i<len; i++)
        uart_putc(*s++);

    return len;
}


void uart_init(int baudrate)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
    RCC->APB1ENR |= RCC_APB1Periph_USART3;

    // Initialize USART
    //
    USART_DeInit(USART3);
    USART_Init(USART3, &(USART_InitTypeDef) {
        .USART_BaudRate = baudrate,
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

    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = USART3_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 15,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART3, ENABLE);
}

