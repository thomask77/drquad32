#pragma once

#include "syscalls.h"

extern struct file_ops  term_cobs_ops;


// TODO: move to  open - _ops
//
void term_cobs_init(void);

