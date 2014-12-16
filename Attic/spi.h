#ifndef SPI_H
#define SPI_H

#include "stm32f4xx.h"

void  spi_begin(GPIO_TypeDef *gpio, int pin, SPI_InitTypeDef spiInit);
void  spi_transfer(const void *out, void *in, int len);
void  spi_end(void);

void  spi_init(void);

#endif
