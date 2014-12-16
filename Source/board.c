#include "board.h"
#include "stm32f4xx.h"

void board_set_leds(int leds)
{
#ifdef BOARD_DISCOVERY
    // STM32F4 Discovery
    //
    GPIOD->BSRRH = (~(leds & 15)) << 12;  // clear leds
    GPIOD->BSRRL =   (leds & 15)  << 12;  // set leds
#endif
}


void board_init(void)
{
    // Enable the compensation cell and GPIO clocks
    //
    SYSCFG->CMPCR = SYSCFG_CMPCR_CMP_PD;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

#ifdef BOARD_DISCOVERY
    // PD12  LED_GREEN
    // PD13  LED_ORANGE
    // PD14  LED_RED
    // PD15  LED_BLUE
    //
    GPIO_Init(GPIOD, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15,
        .GPIO_Mode  = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_OType = GPIO_OType_PP
    });
#else
#endif
}


