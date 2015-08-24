#pragma once

#include "rc_input.h"

struct rc_ppm_sum_config {
    int expected_channels;  // number of expected channels
    int polarity;           // idle level [0, 1]
};

extern struct rc_ppm_sum_config rc_ppm_sum_config;

void rc_ppm_sum_update(void);
void rc_ppm_sum_init(void);
