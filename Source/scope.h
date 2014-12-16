#pragma once

#include <stdbool.h>

// TODO
//
// enum sample_mode {
//     SAMPLE_MODE_NORMAL,
//     SAMPLE_MODE_AVG,
//     SAMPLE_MODE_MIN_MAX,
// };
//
// enum sample_mode  mode;
// float value, min, max;
//
// TODO
//
// uint32_t  trig_holdoff
//

enum data_type {
    DATA_TYPE_NONE,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_INT8,
    DATA_TYPE_UINT8,
    DATA_TYPE_INT16,
    DATA_TYPE_UINT16,
    DATA_TYPE_INT32,
    DATA_TYPE_UINT32,
    DATA_TYPE_CALLBACK
};


enum trig_mode {
    TRIG_MODE_MANUAL,
    TRIG_MODE_RISE,
    TRIG_MODE_FALL,
    TRIG_MODE_BOTH
};


enum trig_state {
    TRIG_STATE_STOP,
    TRIG_STATE_PRE,
    TRIG_STATE_WAIT,
    TRIG_STATE_POST
};


struct scope_channel {
    const char *name;
    enum data_type  type;
    void    *data;
    void    *state;
    float   value;
};


struct scope {
    // Channel config
    //
    int         num_channels;
    struct scope_channel *channels;

    // Sample buffer
    // At least (num_samples * num_channels * sizeof(float)) bytes
    //
    int         num_samples;
    float       *buffer;

    // Timebase
    //
    int         interval;

    // Trigger
    //
    enum trig_mode  trig_mode;
    int         trig_channel;
    float       trig_level;
    int         trig_auto;          // auto trigger timeout
    int         trig_offset;        // trigger position in recorded data [-inf .. num_samples]

    // Output data
    //
    enum trig_state trig_state;
    int         trig_index;

    // Internal state
    //
    bool        _trig_manual;
    float       _trig_value_old;

    int         _interval_delay;
    int         _auto_delay;
    int         _pre_delay;
    int         _post_delay;
};


// Function prototype for DATA_TYPE_CALLBACK
//
typedef void (*scope_channel_cb)(struct scope*, int);

void scope_exec(struct scope *s);
void scope_trig(struct scope *s);

void scope_start(struct scope *s);

