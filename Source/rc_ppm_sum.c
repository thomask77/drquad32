/**
 * PPM sum signal decoder
 *
 */
#include "rc_ppm_sum.h"
#include "util.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>


#define MIN_WIDTH   (1500 - 750)
#define MAX_WIDTH   (1500 + 750)
#define SYNC_WIDTH  3000

volatile int foo;


struct rc_ppm_sum_config rc_ppm_sum_config;


void TIM8_BRK_TIM12_IRQHandler(void)
{
    static bool valid;
    static int  rssi;
    static int  num_channels;
    static int  channels[RC_MAX_CHANNELS];

    static uint16_t t0;

    uint16_t status = TIM12->SR;

    if (status & TIM_SR_CC2IF) {
        uint16_t t = TIM12->CCR2 - t0;

        if (t < MIN_WIDTH) {
            // Ignore short glitches and log them as
            // an indicator of bad RF signal quality
            //
            if (t > 0 && rssi > 0)
                rssi--;
        }
        else {
            if (t < MAX_WIDTH && num_channels < RC_MAX_CHANNELS)
                channels[num_channels++] = t;
            else
                valid = false;

            t0 += t;

            if (t0 < 50000) {
                // Wait for end of frame
                //
                TIM12->ARR = t0 + SYNC_WIDTH;
            }
        }
    }

    if (status & TIM_SR_UIF) {
        // Validate and copy the frame
        //
        if (num_channels != rc_ppm_sum_config.expected_channels)
            valid = false;

        rc_input.timestamp = xTaskGetTickCountFromISR();
        rc_input.valid = valid;
        rc_input.rssi  = rssi;

        for (int i=0; i<8; i++)
            rc_input.channels[i].pulse = channels[i];

        // Reset the frame
        //
        valid = true;
        rssi = 100;
        num_channels = 0;
        for (int i=0; i<8; i++)
            channels[i] = 0;
        t0  = 0;

        // Reset the counter
        //
        TIM12->CR1 &= ~TIM_CR1_CEN;
        TIM12->SR  &= ~TIM_SR_UIF;
        TIM12->ARR  = SYNC_WIDTH;
        TIM12->CNT  = 0;
    }
}


void rc_ppm_sum_update(void)
{
    if (rc_ppm_sum_config.polarity)
        TIM12->CCER |= TIM_CCER_CC2P;   // Falling edge
    else
        TIM12->CCER &= ~TIM_CCER_CC2P;  // Rising edge
}


void rc_ppm_sum_init(void)
{
    // Enable peripheral clocks
    //
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1Periph_TIM12;

    TIM_DeInit(TIM12);

    // Set up TIM12 capture in one pulse mode.
    //
    // The timer is configured in slave mode, and will start
    // with the first received edge. All following edges will
    // be measured and increment the terminal count by max_width.
    //
    // If the timer expires, the received frame is validated and
    // the timer will be reset to wait for the next frame.
    //
    TIM_TimeBaseInit(TIM12, &(TIM_TimeBaseInitTypeDef) {
        // 1us resolution
        //
        .TIM_Prescaler      = SystemCoreClock/2 / 1000000 - 1,
        .TIM_CounterMode    = TIM_CounterMode_Up,
        .TIM_Period         = MAX_WIDTH,            // TODO: don't.

        // f_DTS = 21 MHz digital filter clock
        //
        .TIM_ClockDivision  = TIM_CKD_DIV4
    } );

    TIM_ARRPreloadConfig(TIM12, DISABLE);
    TIM_SelectOnePulseMode(TIM12, TIM_OPMode_Single);

    TIM_PWMIConfig(TIM12, &(TIM_ICInitTypeDef) {
        .TIM_Channel     = TIM_Channel_2,
        .TIM_ICPolarity  = TIM_ICPolarity_Falling,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,

        // 1100: f_SAMPLING = f_DTS/16, N=8
        //
        .TIM_ICFilter = 12
    } );

    TIM_SelectInputTrigger(TIM12, TIM_TS_TI2FP2);
    TIM_SelectSlaveMode(TIM12, TIM_SlaveMode_Trigger);
    TIM_SelectMasterSlaveMode(TIM12, TIM_MasterSlaveMode_Enable);
    TIM_UpdateRequestConfig(TIM12, TIM_UpdateSource_Regular);

    // Set up GPIO pin
    //
    GPIO_Init(GPIOB, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = GPIO_Pin_15,
        .GPIO_Mode  = GPIO_Mode_AF,
        .GPIO_Speed = GPIO_Low_Speed,
        .GPIO_OType = GPIO_OType_OD,
        .GPIO_PuPd  = GPIO_PuPd_UP
    });

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12);

    // Initialize timer interrupt
    //
    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = TIM8_BRK_TIM12_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
            configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelCmd = ENABLE
    });

    TIM_ITConfig(TIM12, TIM_IT_CC2, ENABLE);
    TIM_ITConfig(TIM12, TIM_IT_Update, ENABLE);
}

