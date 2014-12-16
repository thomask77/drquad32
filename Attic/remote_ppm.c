/**
 * Plain vanilla PPM remote-control decoder
 *
 * \author  Copyright (c)2010 Thomas Kindler <mail@t-kindler.de>
 *
 * This program is free software;  you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License,  or (at your option) any later version.  Read the
 * full License at http://www.gnu.org/copyleft for more details.
 */

// Include files -----
//
#include "remote_ppm.h"
#include "stm32f10x.h"
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>

#define RC_POLARITY         TIM_ICPolarity_Falling
#define RC_GLITCH_PULSE     800
#define RC_SYNC_PULSE       5000


static struct rc_ppm_frame  last_frame;


void  TIM1_CC_IRQHandler(void)
{
    static int  channels[RC_NUM_CHANNELS];
    static int  num_channels;
    static int  glitches;
    static int  last_edge;

    // Clear the interrupt by reading CCR1
    //
    uint16_t  t = TIM1->CCR1 - last_edge;

    // Check overcapture flag
    //
    if (TIM1->SR & TIM_SR_CC1OF) {
        num_channels = 666;
        TIM1->SR    &= ~TIM_SR_CC1OF;
        return;
    }

    // Ignore short glitch pulses and log them
    // as an indicator of RF signal quality
    //
    if (t < RC_GLITCH_PULSE) {
        glitches++;
        return;
    }

    last_edge += t;

    if (t > RC_SYNC_PULSE) {
        if (num_channels == RC_NUM_CHANNELS) {
            memcpy(last_frame.channels, channels, sizeof last_frame.channels);
            last_frame.glitches  = glitches;
            last_frame.timestamp = xTaskGetTickCount();
        }
        num_channels = 0;
        glitches = 0;
        return;
    }

    if (num_channels < RC_NUM_CHANNELS)
        channels[num_channels++] = t - 1500;
    else
        num_channels = 666;
}


void rc_init(void)
{
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_TIM1 |
        RCC_APB2Periph_AFIO |
        RCC_APB2Periph_GPIOA,
        ENABLE
    );

    GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
        .GPIO_Pin   = GPIO_Pin_8,
        .GPIO_Mode  = GPIO_Mode_IPU
    });

    TIM_TimeBaseInit(TIM1, &(TIM_TimeBaseInitTypeDef) {
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Prescaler   = 72-1,  // 1 MHz clock
        .TIM_Period      = 0xffff
    });

    TIM_ICInit(TIM1, &(TIM_ICInitTypeDef) {
        .TIM_Channel     = TIM_Channel_1,
        .TIM_ICPolarity  = RC_POLARITY,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter    = 0x0f
    });

    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    NVIC_Init(&(NVIC_InitTypeDef) {
        .NVIC_IRQChannel = TIM1_CC_IRQn,
        .NVIC_IRQChannelPreemptionPriority =
              configLIBRARY_KERNEL_INTERRUPT_PRIORITY,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    });
}


/**
 * Get remote control frame
 *
 */
void rc_get_frame(struct rc_ppm_frame *rc)
{
    vPortEnterCritical();
    memcpy(rc, &last_frame, sizeof *rc);
    vPortExitCritical();
}


// -------------------- Shell commands --------------------
//
#include "usb_serial.h"
#include "command.h"
#include "misc.h"
#include <stdio.h>


static void cmd_rc_show(void)
{
    struct rc_ppm_frame rc;
    rc_get_frame(&rc);

    printf("dt=%4lu ms, %2d glitches\n", xTaskGetTickCount()-rc.timestamp, rc.glitches);

    for (int i=0; i<RC_NUM_CHANNELS; i++) {
        char bar[31];
        strnbar(bar, sizeof bar, rc.channels[i], -600, 600);
        printf("%2d: %6d   [%s]\n", i, rc.channels[i], bar);
    }
}


SHELL_CMD(rc_show, (cmdfunc_t)cmd_rc_show, "show remote control channels");
