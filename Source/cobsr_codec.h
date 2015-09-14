#pragma once

#include "ringbuf.h"
#include <stddef.h>


struct cobsr_codec {
    struct  ringbuf *rb;

    void    *decoded;
    size_t  decoded_size;
    size_t  decoded_pos;
};

int cobsr_encode_rb(struct ringbuf *rb, const void *data, size_t len);

int cobsr_decode_rb(struct ringbuf *rb, void *data, size_t len);

