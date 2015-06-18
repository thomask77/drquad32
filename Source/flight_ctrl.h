#pragma once


void flight_ctrl(void *pvParameters);


extern float pid_p, pid_i, pid_d;
extern struct pid_ctrl pid_pitch, pid_roll, pid_yaw;
