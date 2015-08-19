#include <stdint.h>
#include "flight_ctrl.h"
#include "sensors.h"
#include "bldc_task.h"
#include "rc_input.h"
#include "util.h"
#include "attitude.h"

#include "FreeRTOS.h"
#include "task.h"


static struct sensor_data   sensor_data;
static struct rc_input      rc_input;

struct pid_ctrl pid_pitch = { .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl pid_roll  = { .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl pid_yaw   = { .min = -5, .max = 5, .dt = 1e-3 };

float rc_pitch, rc_roll, rc_yaw, rc_thrust;

float pid_p = 1;
float pid_i = 0;
float pid_d = 0;

struct s_fc_config fc_config;


/*
void systick_handler(void)
{
    dcm_update(
    sensor_data.gyro,
    sensor_data.acc,
    1.0 / SYSTICK_FREQ
    );

    flt_pid_update(&pid_pitch, (rc_pitch - dcm.euler.x), 0);
    flt_pid_update(&pid_roll , (rc_roll  - dcm.euler.y), 0);
    flt_pid_update(&pid_yaw  , (rc_yaw   - dcm.euler.z), 0);

    int pwm_front = rc_thrust + pid_pitch.out + pid_yaw.out;
    int pwm_back  = rc_thrust - pid_pitch.out + pid_yaw.out;
    int pwm_left  = rc_thrust + pid_roll.out  - pid_yaw.out;
    int pwm_right = rc_thrust - pid_roll.out  - pid_yaw.out;

    motor[ID_FRONT].pwm = clamp(pwm_front, 32, 400);
    motor[ID_BACK ].pwm = clamp(pwm_back , 32, 400);
    motor[ID_LEFT ].pwm = clamp(pwm_left , 32, 400);
    motor[ID_RIGHT].pwm = clamp(pwm_right, 32, 400);
}
*/
void flight_ctrl(void *pvParameters)
{
    //uint32_t t0 = xTaskGetTickCount();

    bldc_state.motors[ID_FL].u_d = 1;
    bldc_state.motors[ID_FR].u_d = 1;
    bldc_state.motors[ID_RL].u_d = 1;
    bldc_state.motors[ID_RR].u_d = 1;

    vTaskDelay(1000);

    int ok = 0;
    int old_ok = 0;
    int stop_once = 0;

    dcm_reset();

    for (;;) {
        sensor_read(&sensor_data);
        rc_update(&rc_input);

        if (rc_input.valid && (rc_input.mapped_channels[RC_CHANNEL_ARM] > -0.7))
        {
            rc_pitch   = rc_input.mapped_channels[RC_CHANNEL_PITCH] * fc_config.pitch_roll_gain;
            rc_roll    = rc_input.mapped_channels[RC_CHANNEL_ROLL] * fc_config.pitch_roll_gain;
            rc_yaw     = rc_input.mapped_channels[RC_CHANNEL_YAW] * fc_config.yaw_gain;
            // go from -1 - 1 to 0 - 1
            rc_thrust  = ((rc_input.mapped_channels[RC_CHANNEL_THURST] * 0.5) + 0.5) * fc_config.thurst_gain;

            ok = 1;
        }
        else {
            rc_pitch    = 0;
            rc_roll     = 0;
            rc_yaw      = 0;
            rc_thrust   = 0;

            ok = 0;
        }

        if (fc_config.state & 0x01) {
            pid_pitch.kp = pid_p;
            pid_roll .kp = pid_p;
            pid_yaw  .kp = pid_p;

            pid_pitch.ki = pid_i;
            pid_roll .ki = pid_i;
            pid_yaw  .ki = pid_i;

            pid_pitch.kd = pid_d;
            pid_roll .kd = pid_d;
            pid_yaw  .kd = pid_d;
        }

        dcm_update(&sensor_data, 1e-3);


        if (fc_config.state & 0x02 || (rc_input.mapped_channels[RC_CHANNEL_FUNCT0] > -0.7)) {
                pid_update(&pid_pitch, (rc_pitch - dcm.euler.x), 0);
                pid_update(&pid_roll , (rc_roll  + dcm.euler.y), 0);
                pid_update(&pid_yaw  , (rc_yaw   + dcm.euler.z), 0);
        } else {
                pid_update(&pid_pitch, (rc_pitch - sensor_data.gyro.x), 0);
                pid_update(&pid_roll , (rc_roll  + sensor_data.gyro.y), 0);
                pid_update(&pid_yaw  , (rc_yaw   + sensor_data.gyro.z), 0);
        }
        if (ok) {
            bldc_state.motors[ID_FL].u_d = clamp(rc_thrust + pid_pitch.u - pid_roll.u - pid_yaw.u, 1, 10);
            bldc_state.motors[ID_FR].u_d = clamp(rc_thrust + pid_pitch.u + pid_roll.u + pid_yaw.u, 1, 10);
            bldc_state.motors[ID_RL].u_d = clamp(rc_thrust - pid_pitch.u - pid_roll.u + pid_yaw.u, 1, 10);
            bldc_state.motors[ID_RR].u_d = clamp(rc_thrust - pid_pitch.u + pid_roll.u - pid_yaw.u, 1, 10);

            if (!old_ok) {
                bldc_state.motors[ID_FL].state = STATE_START;
                bldc_state.motors[ID_FR].state = STATE_START;
                bldc_state.motors[ID_RL].state = STATE_START;
                bldc_state.motors[ID_RR].state = STATE_START;
            }
            stop_once = 0;
        }
        else if (!stop_once) {
            bldc_state.motors[ID_FL].state = STATE_STOP;
            bldc_state.motors[ID_FR].state = STATE_STOP;
            bldc_state.motors[ID_RL].state = STATE_STOP;
            bldc_state.motors[ID_RR].state = STATE_STOP;
            stop_once = 1;
        }

        old_ok = ok;

        vTaskDelay(1);
    }
}
