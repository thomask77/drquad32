#pragma once

#include "matrix3f.h"

struct dcm {
    mat3f matrix;       ///< current direction cosine matrix
    vec3f euler;        ///< roll, pitch and yaw angles
    vec3f omega;        ///< drift corrected angular rates

    vec3f offset_p;     ///< drift correction p-term
    vec3f offset_i;     ///< drift correction i-term

    vec3f down_ref;     ///< accelerometer "down" reference
    float acc_kp;       ///< accelerometer p gain
    float acc_ki;       ///< accelerometer i gain

    vec3f north_ref;    ///< magnetometer "north" reference
    float mag_kp;       ///< magnetometer p gain
    float mag_ki;       ///< magnetometer i gain

    vec3f debug;
};

extern struct dcm dcm;

extern void dcm_update(vec3f gyro, vec3f acc, float dt);
extern void dcm_reset(void);

extern void cmd_dcm_show(void);

