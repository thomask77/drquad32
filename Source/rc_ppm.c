/**
 * PPM sum signal decoder
 *
 */
#include "rc_ppm.h"
#include "util.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

struct rc_ppm_config rc_ppm_config;
volatile uint32_t  rc_ppm_irq_count;
volatile uint32_t  rc_ppm_irq_time;

// Local copy of the last received frame
//
static struct rc_input rc_ppm_input;
static int    ppm_channels[RC_MAX_PHYS_CHANNELS];    // pulse width [us]



void TIM8_BRK_TIM12_IRQHandler(void)
{
    static bool valid;
    static int  rssi;
    static int  num_channels;
    static int  channels[RC_MAX_PHYS_CHANNELS];
    static uint16_t t0;

    uint16_t tim7_cnt = TIM7->CNT;
    uint16_t status = TIM12->SR;

    if (status & TIM_SR_CC2IF) {
        uint16_t t = TIM12->CCR2 - t0;

        if (t < rc_ppm_config.min_width) {
            // Ignore short glitches and log them as
            // an indicator of bad RF signal quality
            //
            if (t > 0 && rssi > 0)
                rssi--;
        }
        else {
            if (t < rc_ppm_config.max_width && num_channels < RC_MAX_PHYS_CHANNELS)
                channels[num_channels++] = t;
            else
                valid = false;

            t0 += t;

            if (t0 < 50000) {
                // Wait for end of frame
                //
                TIM12->ARR = t0 + rc_ppm_config.sync_width;
            }
        }
    }

    if (status & TIM_SR_UIF) {
        // Validate and copy the frame
        //
        if (num_channels != rc_config.expected_channels)
            valid = false;

        rc_ppm_input.timestamp = xTaskGetTickCountFromISR();
        rc_ppm_input.valid = valid;
        rc_ppm_input.num_channels = num_channels;
        rc_ppm_input.rssi  = rssi;
        memcpy(ppm_channels, channels, sizeof(channels));

        // Reset the frame
        //
        valid = true;
        rssi = 100;
        num_channels = 0;
        memset(channels, 0, sizeof(channels));
        t0  = 0;

        // Reset the counter
        //
        TIM12->CR1 &= ~TIM_CR1_CEN;
        TIM12->SR  &= ~TIM_SR_UIF;
        TIM12->ARR  = rc_ppm_config.sync_width;
        TIM12->CNT  = 0;
    }

    rc_ppm_irq_count++;
    rc_ppm_irq_time = (uint16_t)(TIM7->CNT - tim7_cnt);
}


void rc_ppm_update(struct rc_input *rc)
{
    taskDISABLE_INTERRUPTS();

    // Timeout after 100ms
    //
    if (xTaskGetTickCount() - rc_ppm_input.timestamp > 100)
        rc_ppm_input.valid = false;

    rc->num_channels = rc_ppm_input.num_channels;
    rc->rssi = rc_ppm_input.rssi;
    rc->timestamp = rc_ppm_input.timestamp;
    rc->valid = rc_ppm_input.valid;

    for (uint8_t i = 0; i < rc_ppm_input.num_channels; i++)
        rc->channels[i] = (ppm_channels[i] - 1500) / 500.0;

    taskENABLE_INTERRUPTS();

    if (rc_ppm_config.polarity)
        TIM12->CCER |= TIM_CCER_CC2P;   // Falling edge
    else
        TIM12->CCER &= ~TIM_CCER_CC2P;  // Rising edge
}


void rc_ppm_init(void)
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
        .TIM_Period         = rc_ppm_config.max_width,

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

