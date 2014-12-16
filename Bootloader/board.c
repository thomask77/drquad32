#include "board.h"
#include "stm32f4xx.h"
#include <string.h>

volatile uint32_t tickcount;

void SysTick_Handler(void)
{
    tickcount++;
}


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
    // Copy interrupt vectors to RAM
    //
    extern void *g_pfnVectors[];

    static void *g_pfnVectors_RAM[128]
        __attribute__((aligned(512)));

    memcpy(g_pfnVectors_RAM, g_pfnVectors, sizeof(g_pfnVectors_RAM));
    SCB->VTOR = (uint32_t)g_pfnVectors_RAM;

    // Same interrupt priority as for FreeRTOS
    //
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);


    // Enable the compensation cell
    //
    SYSCFG->CMPCR = SYSCFG_CMPCR_CMP_PD;

    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;


#ifdef BOARD_DISCOVERY
    // STM32F4 Discovery:
    //   PD12  LED_GREEN
    //   PD13  LED_ORANGE
    //   PD14  LED_RED
    //   PD15  LED_BLUE
    //
    GPIO_Init(GPIOD, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15,
        .GPIO_Mode  = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_OType = GPIO_OType_PP
    });
#endif


    // Set up 1ms systick interrupt
    //
    SysTick_Config(SystemCoreClock / 1000);
}


void board_deinit(void)
{
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    RCC_DeInit();
    USART_DeInit(USART3);
}

