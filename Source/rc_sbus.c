/**
 * Futaba S.Bus decoder
 *
 *
 */
#include "rc_sbus.h"
#include "stm32f4xx.h"
#include <stdio.h>


void rc_sbus_update(struct rc_output *rc)
{

}


void rc_sbus_init(void)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART_DeInit(USART1);

    // S.Bus uses 100000 Baud, 8E2 with inverted signals
    //
    USART_Init(USART3, &(USART_InitTypeDef) {
        .USART_BaudRate = 100000,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_2,
        .USART_Parity = USART_Parity_Even,
        .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
        .USART_Mode = USART_Mode_Rx
    });

    //  Use single-wire mode, because the inverted signal is connected to TX.
    //
    USART_HalfDuplexCmd(USART1, ENABLE);

    // Initialize GPIO pins
    //
    // PA9 USART1_TX   PPM_IN_N
    //
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);

    GPIO_Init(GPIOA, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_9,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });
}
