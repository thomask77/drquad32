#pragma once

#include "rc_input.h"

struct rc_ppm_config {
    int polarity;       // idle level [0, 1]
    int min_width;      // minimum pulse width [us]
    int max_width;      // maximum pulse width [us]
    int sync_width;     // pause between frames [us]
};

extern volatile uint32_t rc_ppm_irq_time;
extern volatile uint32_t rc_ppm_irq_count;

extern struct rc_ppm_config rc_ppm_config;

void rc_ppm_update(struct rc_output *rc);
void rc_ppm_init(void);
