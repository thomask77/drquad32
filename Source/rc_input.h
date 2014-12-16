#pragma once

#include "FreeRTOS.h"
#include <stdbool.h>

#define RC_MAX_CHANNELS     10


enum {
    RC_MODE_NONE,
    RC_MODE_PPM,
    RC_MODE_SERVO,
    RC_MODE_DSM2,
    RC_MODE_SBUS,

    RC_MODE_MAX = RC_MODE_SBUS
};


struct rc_input {
    TickType_t  timestamp;              // Time of last update
    bool  valid;                        // 0, 1
    int   rssi;                         // 0 .. 100
    int   num_channels;                 // 0 .. RC_MAX_CHANNELS-1
    int   channels[RC_MAX_CHANNELS];    // pulse width [us]
};


struct rc_config {
    int     mode;
    int     expected_channels;
};


extern struct rc_config rc_config;

void rc_update(struct rc_input *rc);
void rc_init(void);
