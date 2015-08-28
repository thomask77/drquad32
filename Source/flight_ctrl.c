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

struct pid_ctrl pid_roll  = { .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl pid_pitch = { .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl pid_yaw   = { .min = -5, .max = 5, .dt = 1e-3 };

float rc_pitch, rc_roll, rc_yaw, rc_thrust;

struct fc_config fc_config;


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
        rc_input_update();

        if (rc_input.valid && rc_input.fmod > -0.7) {
            rc_pitch   = rc_input.pitch * fc_config.pitch_roll_gain;
            rc_roll    = rc_input.roll  * fc_config.pitch_roll_gain;
            rc_yaw     = rc_input.yaw   * fc_config.yaw_gain;
            rc_thrust  = (rc_input.thrust * 0.5 + 0.5) * fc_config.thrust_gain;
            ok = 1;
        }
        else {
            rc_pitch    = 0;
            rc_roll     = 0;
            rc_yaw      = 0;
            rc_thrust   = 0;
            ok = 0;

            pid_reset(&pid_roll , 0);
            pid_reset(&pid_pitch, 0);
            pid_reset(&pid_yaw  , 0);
        }

        dcm_update(&sensor_data, 1e-3);


        if (rc_input.hold > -0.7) {
            pid_update(&pid_roll , (rc_roll  - dcm.euler.x), 0);        // angle
            pid_update(&pid_pitch, (rc_pitch - dcm.euler.y), 0);        // angle
            pid_update(&pid_yaw  , (rc_yaw   - dcm.omega.z), 0);        // rate control for yaw
        }
        else {
            pid_update(&pid_roll , (rc_roll  - dcm.omega.x), 0);
            pid_update(&pid_pitch, (rc_pitch - dcm.omega.y), 0);
            pid_update(&pid_yaw  , (rc_yaw   - dcm.omega.z), 0);
        }

        if (ok) {
            bldc_state.motors[ID_FL].u_d = clamp(rc_thrust + pid_pitch.u + pid_roll.u - pid_yaw.u, 1, 10);
            bldc_state.motors[ID_FR].u_d = clamp(rc_thrust + pid_pitch.u - pid_roll.u + pid_yaw.u, 1, 10);
            bldc_state.motors[ID_RL].u_d = clamp(rc_thrust - pid_pitch.u + pid_roll.u + pid_yaw.u, 1, 10);
            bldc_state.motors[ID_RR].u_d = clamp(rc_thrust - pid_pitch.u - pid_roll.u - pid_yaw.u, 1, 10);

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
