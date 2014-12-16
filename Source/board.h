#pragma once

#include <stdint.h>

// Board support
//
// TODO: LED_FL, LED_FR, LED_RL, LED_RR
//
#define LED_GREEN       1
#define LED_ORANGE      2
#define LED_RED         4
#define LED_BLUE        8

extern int  board_address;

void  board_init(void);
void  board_set_leds(int leds);

uint16_t board_get_hvmon(void);
