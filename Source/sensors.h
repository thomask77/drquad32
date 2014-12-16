#pragma once

#include "matrix3f.h"
#include <stdint.h>

#define STANDARD_GRAVITY    9.80665     // [m/s^2]
#define STANDARD_PRESSURE   1013.25     // [hPa]


#define CLIP_ACC_X      1
#define CLIP_ACC_Y      2
#define CLIP_ACC_Z      4
#define CLIP_GYRO_TEMP  8
#define CLIP_GYRO_X     16
#define CLIP_GYRO_Y     32
#define CLIP_GYRO_Z     64
#define CLIP_MAG_X      128
#define CLIP_MAG_Y      256
#define CLIP_MAG_Z      512
#define CLIP_BARO_TEMP  1024
#define CLIP_PRESSURE   2048


struct sensor_calib {
    vec3f   acc_gain;
    vec3f   acc_offset;
    float   temp_gain;
    float   temp_offset;
    vec3f   gyro_gain;
    vec3f   gyro_offset;
    vec3f   mag_gain;
    vec3f   mag_offset;
};


// calibrated sensor data
//
struct sensor_data {
    uint32_t  clipflags;
    vec3f   acc;            // [m/s^2]
    float   gyro_temp;      // [°C]
    vec3f   gyro;           // [rad/s]
    vec3f   mag;            // [T]
    float   pressure;       // [hPa]
    float   baro_temp;      // [°C]
};

extern struct sensor_calib sensor_calib;

void sensor_read(struct sensor_data *d);
void sensor_task(void *param);
