#pragma once

void flight_ctrl(void *pvParameters);


extern struct pid_ctrl pid_pitch, pid_roll, pid_yaw;


struct fc_config {
    float thrust_gain;
    float pitch_roll_gain;
    float yaw_gain;
};

extern struct fc_config fc_config;
