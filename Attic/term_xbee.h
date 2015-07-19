#pragma once

#include "syscalls.h"

extern struct file_ops term_xbee_ops;

void xbee_poll_send(const char *s);
void xbee_init(void);

