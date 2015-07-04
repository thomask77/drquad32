#pragma once

#include <unistd.h>
#include <stdio.h>

ssize_t uart_read(void *buf, size_t len);
ssize_t uart_write(const void *buf, size_t len);

size_t uart_bytes_avail(void);
void uart_flush(void);

void uart_init(int baudrate);
