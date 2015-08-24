#pragma once

#include "util.h"
#include "FreeRTOS.h"
#include <limits.h>
#include <stdbool.h>

#define RC_MAX_CHANNELS     8


enum rc_driver {
    RC_DRIVER_NONE,
    RC_DRIVER_PPM_SUM,
    RC_DRIVER_PPM_SEPARATE,
    // RC_DRIVER_DSM2,
    // RC_DRIVER_SBUS,

    RC_DRIVER_COUNT,
    RC_DRIVER_PADDING = INT_MAX
};


enum rc_function {
    RC_FUNCTION_NONE,
    RC_FUNCTION_PITCH,
    RC_FUNCTION_ROLL,
    RC_FUNCTION_THRUST,
    RC_FUNCTION_YAW,
    RC_FUNCTION_FMOD,
    RC_FUNCTION_HOLD,
    RC_FUNCTION_AUX1,
    RC_FUNCTION_AUX2,

    RC_FUNCTION_COUNT,
    RC_FUNCTION_PADDING = INT_MAX
};


// check for parameter table compatibility
//
STATIC_ASSERT(sizeof(enum rc_driver)   == sizeof(int));
STATIC_ASSERT(sizeof(enum rc_function) == sizeof(int));


struct rc_channel {
    // input value (filled by driver)
    //
    int     pulse;      // [us]

    // calibration settings
    //
    int     center;     // [us]     neutral pulse length
    int     max;        // [us]     maximum pulse length
    int     deadband;   // [us]     deadband around center position
    int     invert;     // [0,1]    invert channel

    // output value
    //
    enum    rc_function  function;
    float   value;      // mapped output value [-1..1]
};


struct rc_input {
    TickType_t  timestamp;
    bool    valid;
    int     rssi;       // [0..100]

    struct  rc_channel  channels[RC_MAX_CHANNELS];

    union {
        float values[RC_FUNCTION_COUNT];
        struct {    // order must match enum
            float   _none;
            float   pitch;
            float   roll;
            float   thrust;
            float   yaw;
            float   fmod;
            float   hold;
            float   aux1;
            float   aux2;
        };
    };
};


struct rc_config {
    enum rc_driver  driver;
};


extern struct rc_input  rc_input;
extern struct rc_config rc_config;

void rc_input_init(void);
void rc_input_update(void);
