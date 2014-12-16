#pragma once

#include "matrix3f.h"
#include <stdint.h>

struct mpu9150_regs {
    uint8_t acc_xout_h, acc_xout_l;
    uint8_t acc_yout_h, acc_yout_l;
    uint8_t acc_zout_h, acc_zout_l;
    uint8_t temp_out_h, temp_out_l;
    uint8_t gyro_xout_h, gyro_xout_l;
    uint8_t gyro_yout_h, gyro_yout_l;
    uint8_t gyro_zout_h, gyro_zout_l;
};

struct mpu9150_data {
    uint32_t clipflags;
    vec3f    acc, gyro;
    float    temp;
};

int mpu9150_read(struct mpu9150_regs *regs);
int mpu9150_convert(struct mpu9150_data *data, const struct mpu9150_regs *regs);
int mpu9150_init(void);
