#include "flight_ctrl.h"
#include "sensors.h"
#include "bldc_task.h"
#include "rc_input.h"
#include "util.h"

#include "FreeRTOS.h"
#include "task.h"


static struct sensor_data   sensor_data;
static struct rc_input      rc_input;

struct pid_ctrl   pid_pitch = { .kp = 1, .ki = 0, .kd = 0, .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl   pid_roll  = { .kp = 1, .ki = 0, .kd = 0, .min = -5, .max = 5, .dt = 1e-3 };
struct pid_ctrl   pid_yaw   = { .kp = 1, .ki = 0, .kd = 0, .min = -5, .max = 5, .dt = 1e-3 };

float   rc_pitch, rc_roll, rc_yaw, rc_thrust;

float foo = 1;
float bar = 0;
float baz = 0;

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

    for (;;) {
        sensor_read(&sensor_data);
        rc_update(&rc_input);

        if (rc_input.valid && rc_input.channels[5] < 1500)
        {
            rc_pitch  = -(rc_input.channels[1] - 1500) / 500.0;
            rc_roll   =  (rc_input.channels[0] - 1500) / 500.0;
            rc_yaw    = -(rc_input.channels[3] - 1500) / 500.0;

            rc_thrust = -(rc_input.channels[2] - 1500) / 1000.0;
            rc_thrust += 0.5;
            rc_thrust *= 5;

            ok = 1;
        }
        else {
            rc_pitch    = 0;
            rc_roll     = 0;
            rc_yaw      = 0;
            rc_thrust   = 0;

            ok = 0;
        }

        pid_pitch.kp = foo;
        pid_roll.kp = foo;
        pid_yaw.kp = foo;

        pid_pitch.ki = bar;
        pid_roll.ki = bar;
        pid_yaw.ki = bar;

        pid_pitch.kd = baz;
        pid_roll.kd = baz;
        pid_yaw.kd = baz;

        pid_update(&pid_pitch, (rc_pitch - sensor_data.gyro.x), 0);
        pid_update(&pid_roll , (rc_roll  + sensor_data.gyro.y), 0);
        pid_update(&pid_yaw  , (rc_yaw   + sensor_data.gyro.z), 0);


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
        }
        else {
            bldc_state.motors[ID_FL].state = STATE_STOP;
            bldc_state.motors[ID_FR].state = STATE_STOP;
            bldc_state.motors[ID_RL].state = STATE_STOP;
            bldc_state.motors[ID_RR].state = STATE_STOP;
        }

        old_ok = ok;

        vTaskDelay(1);
    }
}
