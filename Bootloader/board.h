#pragma once

#include <stdint.h>

#define LED_GREEN       1
#define LED_ORANGE      2
#define LED_RED         4
#define LED_BLUE        8

extern volatile uint32_t tickcount;

void  board_set_leds(int leds);
void  board_init(void);
void  board_deinit(void);
