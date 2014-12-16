#pragma once

#include "matrix3f.h"
#include <stdint.h>

struct ak8975_regs {
    uint8_t st1;
    uint8_t hxl, hxh;
    uint8_t hyl, hyh;
    uint8_t hzl, hzh;
    uint8_t st2;
};


struct ak8975_data {
    uint32_t  clipflags;
    vec3f     mag;
};

int ak8975_start_single(void);
int ak8975_read(struct ak8975_regs *regs);
int ak8975_convert(struct ak8975_data *data, const struct ak8975_regs *regs);
int ak8975_init(void);
