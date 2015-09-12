#pragma once

#include "ringbuf.h"
#include <stddef.h>


struct cobsr_codec {
    struct  ringbuf *encoded;

    void    *decoded;
    size_t  decoded_size;
    size_t  decoded_pos;
};


int cobsr_encode_x(struct cobsr_codec *c);
int cobsr_decode_x(struct cobsr_codec *c);
