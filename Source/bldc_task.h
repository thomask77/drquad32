#pragma once

#include <stdint.h>
#include "bldc_driver.h"
#include "filter.h"

enum {
    STATE_STOP,
    STATE_START,
    STATE_RUNNING,
    STATE_ERROR
};


struct motor_state {
    // Setpoints
    //
    float   u_d;
    int32_t reverse;

    // Values from bldc_get_measurements
    //
    float   u_a, u_b, u_c;

    // Calculated values
    //
    float   u_alpha;
    float   u_beta;
    float   u_null;

    // Internal state
    //
    int     state;
    int     t_state;

    int     pos;

    int     emf;
    int     emf_ok;

    uint32_t    t_step_last;
    uint32_t    t_step_next;
    uint32_t    t_step_timeout;

    struct  lp2_filter rpm_filter;
    int     rpm_old_pos;

    // Values for bldc_set_outputs
    //
    float  		u_pwm;
    int     	step;
    uint8_t     led;
};


struct bldc_state {
    // Values from bldc_get_measurements
    //
    float   u_bat;
    float   u_aux;
    int     thdn;

    int     errors;

    // Motor states
    //
    struct motor_state  motors[4];
};


struct bldc_params
{
    float   dudt_max;

    // Limits
    //
    float   u_bat_min;
    float   u_bat_max;

    // Motor parameters
    //
    int     polepairs;
    float   K_v;

    // BLDC control parameters
    //
    int     t_deadtime;
    int     t_emf_hold_off;
    float   u_emf_hyst;
};


union error_flags {
    struct {
        unsigned  u_bat_min : 1;
        unsigned  u_bat_max : 1;
        unsigned  fet_temp  : 1;
    };
    uint32_t  w;
};


union warning_flags {
    struct {

    };
    uint32_t  w;
};


extern union  error_flags   errors;
extern union  warning_flags warnings;

extern struct bldc_state    bldc_state;
extern struct bldc_params   bldc_params;

void bldc_irq_handler(void);
void bldc_task(void *pvParameters);
