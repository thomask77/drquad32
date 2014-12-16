#pragma once

#include <stdint.h>
#include <stddef.h>

int  i2c_read (uint8_t addr, uint8_t reg, void *data, size_t size);
int  i2c_write(uint8_t addr, uint8_t reg, const void *data, size_t size);

void i2c_init(void);
