#pragma once

#include <unistd.h>
#include <stdio.h>

void printf_putchar(char c);

void uart_putc(char c);
int uart_getc(void);
void uart_flush(void);

ssize_t uart_write(const void *buf, size_t len);

void uart_init(int baudrate);
