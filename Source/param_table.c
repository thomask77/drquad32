#include "param_table.h"
#include "util.h"
#include "bldc_driver.h"
#include "bldc_task.h"
#include "debug_dac.h"
#include "rc_input.h"
#include "rc_ppm.h"
#include "dma_io_driver.h"
#include "sensors.h"
#include "flight_ctrl.h"
#include "attitude.h"

static int board_address;

const struct param_info  param_table[] = {
    {    1, P_INT32(&board_address    ), READONLY, .name = "board_address" },
    {    4, P_INT32((int*)&warnings.w ), NOEEPROM, .name = "warning_flags" },
    {    6, P_INT32((int*)&errors.w   ), NOEEPROM, .name = "error_flags" },

    {   32, P_INT32(&bldc_params.polepairs, 7, 1, 20 ),
            .name = "Polepairs",
            .help = "Number of motor pole pairs"
    },

    {   38, P_FLOAT(&bldc_params.K_v, 700, 0,  10000 ),
            .name = "K_v", .unit = "rpm/V",
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

    {  100, P_FLOAT(&bldc_params.dudt_max, 25, 1, 1000),
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

    {  200, P_INT32(&rc_config.mode, 0, 0, RC_MODE_MAX),
            .name = "rc.mode",
            .help = "Select remote control mode (requires reboot):\n"
                    "  0: No remote control\n"
                    "  1: PPM sum signal\n"
                    "  2: Individual servo channels\n"
                    "  3: Spektrum DSM2 satellite\n"
                    "  4: Futaba SBUS\n"
    },

    {  201, P_INT32(&rc_config.expected_channels, 0, 0, RC_MAX_PHYS_CHANNELS),
            .name = "rc.expected_channels",
            .help = "Number of expected remote control channels.\n"
    },

    {  210, P_INT32(&rc_ppm_config.polarity, 1, 0, 1),
            .name = "rc_ppm.polarity",
            .help = "Polarity of the PPM sum signal\n"
                    "  0: Idle low\n"
                    "  1: Idle high\n"
    },

    {  211, P_INT32(&rc_ppm_config.min_width, 800, 0, 3000),
            .name = "rc_ppm.min_width", .unit = "us",
            .help = "PPM sum signal minimum pulse width"
    },
    {  212, P_INT32(&rc_ppm_config.max_width, 2500, 0, 3000),
            .name = "rc_ppm.max_width", .unit = "us",
            .help = "PPM sum signal maximum pulse width"
    },

    {  213, P_INT32(&rc_ppm_config.sync_width, 3800, 3000, 50000),
            .name = "rc_ppm.sync_width", .unit = "us",
            .help = "PPM sum signal synchronization pulse width"
    },

    {  240, P_INT32(&rc_config.channel_map[RC_CHANNEL_THURST], 2, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[THURST]",
        .help = "Channel with thurst signal"
    },
    {  241, P_INT32(&rc_config.channel_map[RC_CHANNEL_PITCH], 1, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[PITCH]",
        .help = "Channel with pitch signal"
    },
    {  242, P_INT32(&rc_config.channel_map[RC_CHANNEL_ROLL], 0, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[ROLL]",
        .help = "Channel with roll signal"
    },
    {  243, P_INT32(&rc_config.channel_map[RC_CHANNEL_YAW], 3, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[YAW]",
        .help = "Channel with yaw signal"
    },
    {  244, P_INT32(&rc_config.channel_map[RC_CHANNEL_ARM], 4, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[ARM]",
        .help = "Channel with arm signal"
    },
    {  245, P_INT32(&rc_config.channel_map[RC_CHANNEL_FUNCT0], 5, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[FUNCT0]",
        .help = "Channel with function 0 signal"
    },
    {  246, P_INT32(&rc_config.channel_map[RC_CHANNEL_FUNCT1], 6, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[FUNCT1]",
        .help = "Channel with function 1 signal"
    },
    {  247, P_INT32(&rc_config.channel_map[RC_CHANNEL_FUNCT2], 7, 0, RC_MAX_PHYS_CHANNELS-1),
        .name = "rc_config.channel_map[FUNCT1]",
        .help = "Channel with function 2 signal"
    },

    {  250, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_THURST], 0, 0, 1),
        .name = "rc_config.channel_inverted[THURST]",
        .help = "invert channel thurst"
    },
    {  251, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_PITCH], 0, 0, 1),
        .name = "rc_config.channel_inverted[PITCH]",
        .help = "invert channel pitch"
    },
    {  252, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_ROLL], 0, 0, 1),
        .name = "rc_config.channel_inverted[ROLL]",
        .help = "invert channel roll"
    },
    {  253, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_YAW], 0, 0, 1),
        .name = "rc_config.channel_inverted[YAW]",
        .help = "invert channel yaw"
    },
    {  254, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_ARM], 0, 0, 1),
        .name = "rc_config.channel_inverted[ARM]",
        .help = "invert channel arm"
    },
    {  255, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_FUNCT0], 0, 0, 1),
        .name = "rc_config.channel_inverted[FUNCT0]",
        .help = "invert channel funct 0"
    },
    {  256, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_FUNCT1], 0, 0, 1),
        .name = "rc_config.channel_inverted[FUNCT1]",
        .help = "invert channel funct 1"
    },
    {  257, P_INT32((int *) &rc_config.channel_inverted[RC_CHANNEL_FUNCT2], 0, 0, 1),
        .name = "rc_config.channel_inverted[FUNCT2]",
        .help = "invert channel funct 2"
    },

    {  300, P_FLOAT(&pid_p, 1, 0 , 10),
       .name = "pid_p",
       .help = "all gyro PID P Part"
    },
    {  301, P_FLOAT(&pid_i, 0, 0, 10),
       .name = "pid_i",
       .help = "all gyro PID I Part"
    },
    {  302, P_FLOAT(&pid_d, 0, 0, 10),
       .name = "pid_d",
       .help = "all gyro PID D Part"
    },


    {  310, P_FLOAT(&pid_pitch.kp, 1, 0, 10),
       .name = "pid_pitch.kp",
       .help = "gyro pitch PID P Part"
    },
    {  311, P_FLOAT(&pid_pitch.ki, 0, 0 ,10),
       .name = "pid_pitch.ki",
       .help = "gyro pitch PID I Part"
    },
    {  312, P_FLOAT(&pid_pitch.kd, 0, 0, 10),
       .name = "pid_pitch.kd",
       .help = "gyro pitch PID D Part"
    },

    {  320, P_FLOAT(&pid_roll.kp, 1, 0, 10),
        .name = "pid_roll.kp",
        .help = "gyro roll PID P Part"
    },
    {  321, P_FLOAT(&pid_roll.ki, 0, 0, 10),
        .name = "pid_roll.ki",
        .help = "gyro roll PID I Part"
    },
    {  322, P_FLOAT(&pid_roll.kd, 0, 0, 10),
        .name = "pid_roll.kd",
        .help = "gyro roll PID D Part"
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

    {  380, P_FLOAT(&fc_config.thurst_gain, 5, 0.1, 10),
       .name = "fc_config.thurst_gain",
       .help = "gain for thurst"
    },

    {  381, P_FLOAT(&fc_config.pitch_roll_gain, 1, 0.1, 5),
       .name = "fc_config.pitch_roll_gain",
       .help = "gain for pitch and roll"
    },
    {  382, P_FLOAT(&fc_config.yaw_gain, 1, 0.1, 5),
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

    {  500, P_INT32(&ws2812_brightness, 128, 0, 255),
            .name = "ws2812_brightness",
            .help = "Overall brightness for WS2128 leds. Adjust this parameter "
                    "to obey the current limit for long LED chains."
    },

    {  600, P_FLOAT(&sensor_calib.gyro_offset.z) },

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
    { 20001, P_INT32((int*)&bldc_irq_time), READONLY, .unit = "us" },
    { 20002, P_INT32((int*)&bldc_irq_time1), READONLY, .unit = "us" },
    { 20003, P_INT32((int*)&bldc_irq_time2), READONLY, .unit = "us" },
    { 20004, P_INT32((int*)&bldc_irq_time3), READONLY, .unit = "us" },

    { 20010, P_INT32((int*)&rc_ppm_irq_count), READONLY },
    { 20011, P_INT32((int*)&rc_ppm_irq_time), READONLY, .unit = "us" },

    { 20020, P_INT32((int*)&dma_io_irq_count), READONLY },
    { 20021, P_INT32((int*)&dma_io_irq_time), READONLY, .unit = "us" }
};


const int param_count = ARRAY_SIZE(param_table);
