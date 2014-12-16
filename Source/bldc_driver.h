#pragma once

#include <stdint.h>
#include <stdbool.h>

#define BLDC_IRQ_FREQ   20000   // Hz

enum {
    ID_FL, ID_FR,
    ID_RL, ID_RR
};

extern volatile uint32_t bldc_irq_count;
extern volatile uint32_t bldc_irq_time;
extern volatile uint32_t bldc_irq_time1;
extern volatile uint32_t bldc_irq_time2;
extern volatile uint32_t bldc_irq_time3;


void    bldc_driver_init(void);
