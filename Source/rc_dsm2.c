/**
 * Spektrum DSM2 satellite decoder
 *
 * Note: J1 must be set to 3.3V
 *
 */
#include "rc_dsm2.h"
#include "stm32f4xx.h"


void rc_dsm2_update(struct rc_output *rc)
{

}


void rc_dsm2_init(void)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART_DeInit(USART1);

    // DSM2 uses 115200 Baud, 8N1
    //
    USART_Init(USART3, &(USART_InitTypeDef) {
        .USART_BaudRate = 115200,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_1,
        .USART_Parity = USART_Parity_No ,
        .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
        .USART_Mode = USART_Mode_Rx
    });

    // Initialize GPIO pins
    //
    // PA10 USART1_RX   PPM_IN
    //
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_Init(GPIOA, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_10,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });
}

