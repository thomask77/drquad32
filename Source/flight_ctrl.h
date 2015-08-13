#pragma once


void flight_ctrl(void *pvParameters);


extern float pid_p, pid_i, pid_d;
extern struct pid_ctrl pid_pitch, pid_roll, pid_yaw;


struct s_fc_config {
    float thurst_gain;
    float pitch_roll_gain;
    float yaw_gain;
    uint8_t state;
};

extern struct s_fc_config fc_config;
