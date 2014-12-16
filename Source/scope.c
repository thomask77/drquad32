#include "scope.h"
#include <assert.h>
#include <stdint.h>


static void update_channel(struct scope *s, int i)
{
    struct scope_channel *c = &s->channels[i];

    switch (c->type) {
    case DATA_TYPE_FLOAT:   c->value = *(float*)   c->data; break;
    case DATA_TYPE_DOUBLE:  c->value = *(double*)  c->data; break;
    case DATA_TYPE_INT8:    c->value = *(int8_t*)  c->data; break;
    case DATA_TYPE_UINT8:   c->value = *(uint8_t*) c->data; break;
    case DATA_TYPE_INT16:   c->value = *(int16_t*) c->data; break;
    case DATA_TYPE_UINT16:  c->value = *(uint16_t*)c->data; break;
    case DATA_TYPE_INT32:   c->value = *(int32_t*) c->data; break;
    case DATA_TYPE_UINT32:  c->value = *(uint32_t*)c->data; break;
    case DATA_TYPE_CALLBACK:  ((scope_channel_cb)c->data)(s, i); break;
    default: break;
    }
}


static void update_trigger(struct scope *s)
{
    float trig_value = s->channels[s->trig_channel].value;

    if (s->trig_state == TRIG_STATE_PRE && !s->_interval_delay) {
        if (s->_pre_delay)
            s->_pre_delay--;
        else
            s->trig_state = TRIG_STATE_WAIT;
    }

    if (s->trig_state == TRIG_STATE_WAIT) {
        // Manual trigger
        //
        bool trigged = s->_trig_manual;

        // Edge trigger
        //
        bool rise = (s->_trig_value_old < s->trig_level) && (trig_value >= s->trig_level);
        bool fall = (s->_trig_value_old > s->trig_level) && (trig_value <= s->trig_level);

        switch (s->trig_mode) {
        case TRIG_MODE_RISE:  trigged |= rise;           break;
        case TRIG_MODE_FALL:  trigged |= fall;           break;
        case TRIG_MODE_BOTH:  trigged |= rise || fall;   break;
        default: break;
        }

        // Auto trigger
        //
        if (s->trig_auto && !s->_interval_delay) {
            if (s->_auto_delay)
                s->_auto_delay--;
            else
                trigged = 1;
        }

        if (trigged)
            s->trig_state = TRIG_STATE_POST;
    }

    s->_trig_value_old = trig_value;

    if (s->trig_state == TRIG_STATE_POST && !s->_interval_delay) {
        if (s->_post_delay)
            s->_post_delay--;
        else
            s->trig_state = TRIG_STATE_STOP;
    }
}


void scope_exec(struct scope *s)
{
    // Update channels
    //
    for (int i=0; i < s->num_channels; i++)
        update_channel(s, i);

    // Check trigger condition
    //
    update_trigger(s);

    // Record samples
    //
    if (s->trig_state != TRIG_STATE_STOP && !s->_interval_delay) {
        for (int i=0; i < s->num_channels; i++)
            s->buffer[s->trig_index * s->num_channels + i] = s->channels[i].value;

        s->trig_index++;
        if (s->trig_index >= s->num_samples)
            s->trig_index = 0;
    }

    // Update timebase
    //
    if (s->trig_state != TRIG_STATE_STOP) {
        if (s->_interval_delay)
            s->_interval_delay--;
        else
            s->_interval_delay = s->interval;
    }
}


void scope_trig(struct scope *s)
{
    s->_trig_manual = 1;
}


void scope_start(struct scope *s)
{
    // Initialize state machine
    //
    s->trig_state = TRIG_STATE_STOP;

    s->_trig_manual = 0;
    s->_auto_delay = s->trig_auto;
    s->_interval_delay = 0;

    // Calculate pre- and post-delay values
    //
    // trig_offset >= 0: The trigger point lies within the window
    // trig_offset <  0: The trigger point lies to the left of the window
    //
    assert(s->trig_offset <= s->num_samples);

    if (s->trig_offset > 0)
        s->_pre_delay = s->trig_offset;
    else
        s->_pre_delay = 0;

    s->_post_delay = s->num_samples - s->trig_offset;

    s->trig_state = TRIG_STATE_PRE;
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <math.h>


static int cmd_scope(int argc, char *argv[])
{
    if (argc < 2)
        goto usage;

    return 0;

usage:
    printf("%s <show>\n", argv[0]);
    return 0;
}


static void scope_show(struct scope *s, int pretrig)
{
    printf("\t");
    for (int j=0; j < s->num_channels; j++) {
        printf( "%s%s", s->channels[j].name,
            j < s->num_channels - 1 ? "\t" : "\n"
        );
    }

    int index = s->trig_index;

    for (int i=0; i < s->num_samples; i++) {
        printf("%d\t", i);

        for (int j=0; j < s->num_channels; j++) {
            printf( "%f%s", s->buffer[index * s->num_channels + j],
                j < s->num_channels - 1 ? "\t" : "\n"
            );
        }

        index++;
        if (index >= s->num_samples)
            index = 0;
    }
}


float u_a, u_b, u_c;

struct scope scope = {
    .num_channels = 3,
    .channels = (struct scope_channel[]) {
        { .name = "u_a", .type = DATA_TYPE_FLOAT, .data = &u_a },
        { .name = "u_b", .type = DATA_TYPE_FLOAT, .data = &u_b },
        { .name = "u_c", .type = DATA_TYPE_FLOAT, .data = &u_c },
    },
    .num_samples = 1024,
    .buffer = (float[1024 * 3 * 4]) { },
    .interval = 1,
    .trig_mode   = TRIG_MODE_RISE,
    .trig_level  = 0.5,
    .trig_offset = 200
};


static int cmd_scope_test(int argc, char *argv[])
{
    scope_start(&scope);

    float phi = 0;
    while (scope.trig_state != TRIG_STATE_STOP) {
        u_a = sinf(phi);
        u_b = sinf(phi + 120 * M_TWOPI / 360);
        u_c = sinf(phi + 240 * M_TWOPI / 360);

        uint16_t  t0 = TIM7->CNT;
        scope_exec(&scope);
        uint16_t  t = TIM7->CNT - t0;

        printf("%f, %d (%d us)\n", u_a, scope.trig_state, t);

        phi += 10 * M_TWOPI / 20000;
        if (phi > M_TWOPI)
            phi -= M_TWOPI;
    }

    scope_show(&scope, 20);

    return 0;
}


SHELL_CMD(scope,       cmd_scope,       "oscilloscope")
SHELL_CMD(scope_test,  cmd_scope_test,  "scope test function")
