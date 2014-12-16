#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

#define FAULT_INFO_MAGIC   0xDEADBEEF


// TODO - siehe Programming manual
struct stack_frame {
    uint32_t    r0,  r1,  r2,  r3;
    uint32_t    r12, lr,  pc,  xpsr;
    uint32_t    s0,  s1,  s2,  s3;
    uint32_t    s4,  s5,  s6,  s7;
    uint32_t    s8,  s9,  s10, s11;
    uint32_t    s12, s13, s14, s15;
    uint32_t    fpscr;
};


struct fault_info {
    uint32_t  magic;

    uint32_t  r0,  r1, r2, r3;
    uint32_t  r12, lr, pc, psr;
    uint32_t  bfar, cfsr, hfsr, dfsr, afsr;
};

extern struct fault_info fault_info;

void print_fault_info(void);

