#pragma once

#include "FreeRTOS.h"
#include <stdbool.h>

#define RC_MAX_PHYS_CHANNELS     10


enum {
    RC_MODE_NONE,
    RC_MODE_PPM,
    RC_MODE_SERVO,
    RC_MODE_DSM2,
    RC_MODE_SBUS,

    RC_MODE_MAX = RC_MODE_SBUS
};

enum {
    RC_CHANNEL_THURST,
    RC_CHANNEL_PITCH,
    RC_CHANNEL_ROLL,
    RC_CHANNEL_YAW,
    RC_CHANNEL_ARM,
    RC_CHANNEL_FUNCT0,
    RC_CHANNEL_FUNCT1,
    RC_CHANNEL_FUNCT2,

    RC_CHANNEL_MAX_LOGICAL,
};

struct rc_input {
    TickType_t  timestamp;              // Time of last update
    bool  valid;                        // 0, 1
    int   rssi;                         // 0 .. 100
    int   num_channels;                 // 0 .. RC_MAX_CHANNELS-1
    float channels[RC_MAX_PHYS_CHANNELS];    // scaled
    float mapped_channels[RC_CHANNEL_MAX_LOGICAL];
};

struct rc_config {
    int mode;
    int expected_channels;
    int channel_map[RC_CHANNEL_MAX_LOGICAL];
    int channel_inverted[RC_MAX_PHYS_CHANNELS];
};


extern struct rc_config rc_config;

void rc_update(struct rc_input *rc);
void rc_init(void);
