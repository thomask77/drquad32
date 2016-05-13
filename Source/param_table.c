#include <rc_ppm_sum.h>
#include "param_table.h"
#include "util.h"
#include "bldc_driver.h"
#include "bldc_task.h"
#include "debug_dac.h"
#include "rc_input.h"

#include "sensors.h"
#include "flight_ctrl.h"
#include "attitude.h"

static int board_address;

extern int foo;

char ccc;

const struct param_info  param_table[] = {
    
    {   20, P_TEST(&board_address), READONLY, .name = "woohoo!" },

    {    1, P_INT32(&board_address    ), READONLY, .name = "board_address" },
    {    4, P_INT32((int*)&warnings.w ), NOEEPROM, .name = "warning_flags" },
    {    6, P_INT32((int*)&errors.w   ), NOEEPROM, .name = "error_flags" },

    {   32, P_INT32(&bldc_params.polepairs, 7, 1, 20 ),
            .name = "Polepairs",
            .help = "Number of motor pole pairs"
    },

    {   38, P_FLOAT(&bldc_params.K_v, 700, 0,  10000 ),
            .name = "K_v", .unit = "RPM/V",
            .help = "Motor velocity constant"
    },

    {   42, P_INT32(&bldc_params.t_emf_hold_off, 2, 0, 20),
            .name = "t_emf_hold_off", .unit = "50us",
            .help = "Back-EMF detection hold-off time after a commutation"
    },

    {   43, P_FLOAT(&bldc_params.u_emf_hyst, 0.1, -2, 2),
            .name = "u_emf_hyst", .unit = "V",
            .help = "Back-EMF detection hysteresis voltage"
    },

    {   44, P_INT32(&bldc_params.t_deadtime, 3, 0, 20),
            .name = "t_deadtime", .unit = "50us",
            .help = "Deadtime between ADC measurements and PWM output"
    },

    {  100, P_FLOAT(&bldc_params.dudt_max, 100, 1, 1000),
            .name = "dudt_max", .unit = "V/s",
            .help = "Maximum slew rate of the motor voltage"
    },

    {  102, P_FLOAT(&bldc_params.u_bat_min,  9.0, 6, 18),
            .name = "u_bat_min", .unit = "V",
            .help = "Critical minimum battery voltage. The motors are forced "
                    "into free-wheel mode to prevent further battery discharge."
                    "3S LiPo 9V, 4S LiPo: 12V"
    },

    {  103, P_FLOAT(&bldc_params.u_bat_max, 16.8, 6, 18),
            .name = "u_bat_max", .unit = "V",
            .help = "Critical maximum battery voltage. The motors are forced "
                    "into brake mode to prevent further voltage rise."
    },

    {  200, P_INT32((int*)&rc_config.driver, 0, 0, RC_DRIVER_COUNT - 1),
            .name = "rc.driver",
            .help = "Select remote control driver (requires reboot):\n"
                    "  0: No remote control\n"
                    "  1: PPM sum signal\n"
                    "  2: Separate servo channels\n"
                    // "  3: Spektrum DSM2 satellite\n"
                    // "  4: Futaba SBUS\n"
    },

    {  201, P_INT32(&rc_ppm_sum_config.expected_channels, 8, 0, RC_MAX_CHANNELS),
            .name = "rc_ppm_sum.expected_channels",
            .help = "Number of expected remote control channels.\n"
    },

    {  210, P_INT32(&rc_ppm_sum_config.polarity, 1, 0, 1),
            .name = "rc_ppm_sum.polarity",
            .help = "Polarity of the PPM sum signal\n"
                    "  0: Idle low\n"
                    "  1: Idle high\n"
    },


    {  310, P_FLOAT(&pid_roll.kp, 3, 0, 10),
        .name = "pid_roll.kp",
        .help = "gyro roll PID P Part"
    },
    {  311, P_FLOAT(&pid_roll.ki, 1, 0, 10),
        .name = "pid_roll.ki",
        .help = "gyro roll PID I Part"
    },
    {  312, P_FLOAT(&pid_roll.kd, 1, 0, 10),
        .name = "pid_roll.kd",
        .help = "gyro roll PID D Part"
    },

    {  320, P_FLOAT(&pid_pitch.kp, 3, 0, 10),
       .name = "pid_pitch.kp",
       .help = "gyro pitch PID P Part"
    },
    {  321, P_FLOAT(&pid_pitch.ki, 1, 0 ,10),
       .name = "pid_pitch.ki",
       .help = "gyro pitch PID I Part"
    },
    {  322, P_FLOAT(&pid_pitch.kd, 1, 0, 10),
       .name = "pid_pitch.kd",
       .help = "gyro pitch PID D Part"
    },

    {  330, P_FLOAT(&pid_yaw.kp, 1, 0, 10),
        .name = "pid_yaw.kp",
        .help = "gyro yaw PID P Part"
    },

    {  331, P_FLOAT(&pid_yaw.ki, 0, 0, 10),
        .name = "pid_yaw.ki",
        .help = "gyro yaw PID I Part"
    },

    {  332, P_FLOAT(&pid_yaw.kd, 0, 0, 10),
        .name = "pid_yaw.kd",
        .help = "gyro yaw PID D Part"
    },

    {  360, P_FLOAT(&dcm.acc_kp, 1, 0, 10),
        .name = "dcm.acc_kp",
        .help = "DCM acc P Part"
    },

    {  361, P_FLOAT(&dcm.acc_ki, 0.0001, 0, 10),
       .name = "dcm.acc_ki",
       .help = "DCM acc I Part"
    },

    {  380, P_FLOAT(&fc_config.thrust_gain, 5, 0.1, 10),
       .name = "fc_config.thrust_gain",
       .help = "gain for thrust"
    },

    {  381, P_FLOAT(&fc_config.pitch_roll_gain, 1, 0.1, 10),
       .name = "fc_config.pitch_roll_gain",
       .help = "gain for pitch and roll"
    },

    {  382, P_FLOAT(&fc_config.yaw_gain, 2, 0.1, 10),
       .name = "fc_config.yaw_gain",
       .help = "gain for yaw"
    },


    // Debug DAC outputs
    //
    {  410, P_INT32(&dac_config.dac1_id, 1020),
            .name = "dac1_id",
            .help = "Parameter ID for DAC channel 1"
    },

    {  411, P_FLOAT(&dac_config.dac1_scale, 5),
            .name = "dac1_scale" , .unit = "Units/V",
            .help = "Scale factor for DAC channel 1"
    },

    {  412, P_FLOAT(&dac_config.dac1_offset, 0),
            .name = "dac1_offset", .unit = "Units",
            .help = "Offset for DAC channel 1"
    },

    {  420, P_INT32(&dac_config.dac2_id, 1021),
            .name = "dac2_id",
            .help = "Parameter ID for DAC channel 2"
    },

    {  421, P_FLOAT(&dac_config.dac2_scale, 5),
            .name = "dac2_scale" , .unit = "Units/V",
            .help = "Scale factor for DAC channel 1"
    },

    {  422, P_FLOAT(&dac_config.dac2_offset, 0),
            .name = "dac2_offset", .unit = "Units",
            .help = "Offset for DAC channel 1"
    },

/*
    {  500, P_INT32(&ws2812_brightness, 128, 0, 255),
            .name = "ws2812_brightness",
            .help = "Overall brightness for WS2128 leds. Adjust this parameter "
                    "to obey the current limit for long LED chains."
    },
*/

#define RC_CHANNEL(ID, CH)  \
    { ID + CH*10 + 0, P_INT32(&rc_input.channels[CH].pulse, 1500, 0, 3000), READONLY, \
        .name = "ch" #CH ".pulse", .unit = "us", \
        .help = !ID ? "" : "Measured pulse width" \
    }, \
    { ID + CH*10 + 1, P_INT32(&rc_input.channels[CH].center, 1500, 0, 3000), \
        .name = "ch" #CH ".center"  , .unit="us", \
        .help = !ID ? "" : "Pulse width in center position" \
    }, \
    { ID + CH*10 + 2, P_INT32(&rc_input.channels[CH].max, 1900, 0, 3000), \
        .name = "ch" #CH ".max"     , .unit="us", \
        .help = !ID ? "" : "Pulse width in max position" \
    }, \
    { ID + CH*10 + 3, P_INT32(&rc_input.channels[CH].deadband, 10, 0, 3000), \
        .name = "ch" #CH ".deadband", .unit="us", \
        .help = !ID ? "" : "Deadband around cnter position" \
    }, \
    { ID + CH*10 + 4, P_INT32(&rc_input.channels[CH].invert, CH != 2, 0, 1), \
        .name = "ch" #CH ".invert", .unit="0, 1", \
        .help = !ID ? "" : "Invert channel" \
    }, \
    { ID + CH*10 + 5, P_INT32((int*)&rc_input.channels[CH].function, CH+1, 0, RC_FUNCTION_COUNT-1), \
        .name = "ch" #CH ".function", \
        .help = !ID ? "" : "Select channel function" \
                           "  0: NONE"      \
                           "  1: PITCH"     \
                           "  2: ROLL"      \
                           "  3: THRUST"    \
                           "  4: YAW"       \
                           "  5: FMOD"      \
                           "  6: HOLD"      \
                           "  7: AUX1"      \
                           "  8: AUX2"      \
    }, \
    { ID + CH*10 + 6, P_FLOAT(&rc_input.channels[CH].value, 0, -1, 1), NOEEPROM, \
        .name = "ch" #CH ".value", .unit="-1 .. 1", \
        .help = !ID ? "" : "Output value" \
    }

    RC_CHANNEL(800, 0),
    RC_CHANNEL(800, 1),
    RC_CHANNEL(800, 2),
    RC_CHANNEL(800, 3),
    RC_CHANNEL(800, 4),
    RC_CHANNEL(800, 5),
    RC_CHANNEL(800, 6),
    RC_CHANNEL(800, 7),
#undef RC_CHANNEL

    { 1000, P_FLOAT(&bldc_state.motors[0].u_d, 0, -25, 25 ), NOEEPROM },
    { 1001, P_FLOAT(&bldc_state.motors[0].u_pwm, 0, -25, 25 ), NOEEPROM },
    { 1004, P_INT32(&bldc_state.motors[0].step, 0, 0, 7), NOEEPROM },
    { 1006, P_INT32(&bldc_state.motors[0].emf_ok), NOEEPROM },
    { 1007, P_INT32(&bldc_state.motors[0].state, 1 ) },
    { 1008, P_INT32(&bldc_state.motors[0].reverse, 0, 0, 1 ) },
    { 1010, P_FLOAT(&bldc_state.motors[0].u_a), .unit = "V", READONLY },
    { 1011, P_FLOAT(&bldc_state.motors[0].u_b), .unit = "V", READONLY },
    { 1012, P_FLOAT(&bldc_state.motors[0].u_c), .unit = "V", READONLY },
    { 1020, P_FLOAT(&bldc_state.motors[0].u_alpha), .unit = "V", READONLY },
    { 1021, P_FLOAT(&bldc_state.motors[0].u_beta),  .unit = "V", READONLY },
    { 1022, P_FLOAT(&bldc_state.motors[0].u_null),  .unit = "V", READONLY },
    { 1040, P_FLOAT(&bldc_state.motors[0].rpm_filter.y[0]),  .unit = "rpm", READONLY },

    { 2000, P_FLOAT(&bldc_state.motors[1].u_d, 0, -25, 25 ), NOEEPROM },
    { 2001, P_FLOAT(&bldc_state.motors[1].u_pwm, 0, -25, 25 ), NOEEPROM },
    { 2004, P_INT32(&bldc_state.motors[1].step, 0, 0, 7), NOEEPROM },
    { 2006, P_INT32(&bldc_state.motors[1].emf_ok), NOEEPROM },
    { 2007, P_INT32(&bldc_state.motors[1].state, 1) },
    { 2008, P_INT32(&bldc_state.motors[1].reverse, 0, 0, 1 ) },
    { 2010, P_FLOAT(&bldc_state.motors[1].u_a), .unit = "V", READONLY },
    { 2011, P_FLOAT(&bldc_state.motors[1].u_b), .unit = "V", READONLY },
    { 2012, P_FLOAT(&bldc_state.motors[1].u_c), .unit = "V", READONLY },
    { 2020, P_FLOAT(&bldc_state.motors[1].u_alpha), .unit = "V", READONLY },
    { 2021, P_FLOAT(&bldc_state.motors[1].u_beta),  .unit = "V", READONLY },
    { 2022, P_FLOAT(&bldc_state.motors[1].u_null),  .unit = "V", READONLY },
    { 2040, P_FLOAT(&bldc_state.motors[1].rpm_filter.y[0]),  .unit = "rpm", READONLY },

    { 3000, P_FLOAT(&bldc_state.motors[2].u_d, 0, -25, 25 ), NOEEPROM },
    { 3001, P_FLOAT(&bldc_state.motors[2].u_pwm, 0, -25, 25 ), NOEEPROM },
    { 3004, P_INT32(&bldc_state.motors[2].step, 0, 0, 7), NOEEPROM },
    { 3006, P_INT32(&bldc_state.motors[2].emf_ok), NOEEPROM },
    { 3007, P_INT32(&bldc_state.motors[2].state, 1) },
    { 3008, P_INT32(&bldc_state.motors[2].reverse, 0, 0, 1 ) },
    { 3010, P_FLOAT(&bldc_state.motors[2].u_a), .unit = "V", READONLY },
    { 3011, P_FLOAT(&bldc_state.motors[2].u_b), .unit = "V", READONLY },
    { 3012, P_FLOAT(&bldc_state.motors[2].u_c), .unit = "V", READONLY },
    { 3020, P_FLOAT(&bldc_state.motors[2].u_alpha), .unit = "V", READONLY },
    { 3021, P_FLOAT(&bldc_state.motors[2].u_beta),  .unit = "V", READONLY },
    { 3022, P_FLOAT(&bldc_state.motors[2].u_null),  .unit = "V", READONLY },
    { 3040, P_FLOAT(&bldc_state.motors[2].rpm_filter.y[0]),  .unit = "rpm", READONLY },

    { 4000, P_FLOAT(&bldc_state.motors[3].u_d, 0, -25, 25 ), NOEEPROM },
    { 4001, P_FLOAT(&bldc_state.motors[3].u_pwm, 0, -25, 25 ), NOEEPROM },
    { 4004, P_INT32(&bldc_state.motors[3].step, 0, 0, 7), NOEEPROM },
    { 4006, P_INT32(&bldc_state.motors[3].emf_ok), NOEEPROM },
    { 4007, P_INT32(&bldc_state.motors[3].state, 1) },
    { 4008, P_INT32(&bldc_state.motors[3].reverse, 0, 0, 1 ) },
    { 4010, P_FLOAT(&bldc_state.motors[3].u_a), .unit = "V", READONLY },
    { 4011, P_FLOAT(&bldc_state.motors[3].u_b), .unit = "V", READONLY },
    { 4012, P_FLOAT(&bldc_state.motors[3].u_c), .unit = "V", READONLY },
    { 4020, P_FLOAT(&bldc_state.motors[3].u_alpha), .unit = "V", READONLY },
    { 4021, P_FLOAT(&bldc_state.motors[3].u_beta),  .unit = "V", READONLY },
    { 4022, P_FLOAT(&bldc_state.motors[3].u_null),  .unit = "V", READONLY },
    { 4040, P_FLOAT(&bldc_state.motors[3].rpm_filter.y[0]),  .unit = "rpm", READONLY },

    { 20000, P_INT32((int*)&bldc_irq_count), READONLY },
    { 20001, P_INT32((int*)&bldc_irq_time),  READONLY, .unit = "us" },
    { 20002, P_INT32((int*)&bldc_irq_time1), READONLY, .unit = "us" },
    { 20003, P_INT32((int*)&bldc_irq_time2), READONLY, .unit = "us" },
    { 20004, P_INT32((int*)&bldc_irq_time3), READONLY, .unit = "us" },

    { 30000, P_INT32(&foo), READONLY }
};


const int param_count = ARRAY_SIZE(param_table);
