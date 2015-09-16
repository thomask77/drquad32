#pragma once

#include "ringbuf.h"
#include <stddef.h>

int cobsr_encode_rb(struct ringbuf *rb, const void *data, size_t len);
int cobsr_decode_rb(struct ringbuf *rb, void *data, size_t len);

