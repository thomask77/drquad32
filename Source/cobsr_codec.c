/**
 * State-machine based COBS/R codec for communication packet framing.
 *
 * Copyright (c)2015 Thomas Kindler <mail_cobsr@t-kindler.de>
 *
 * 2015-11-11: tk, new implementation for segmented input/output buffers.
 * 2015-09-20: tk, added documentation and copyright header.
 * 2015-09-19: tk, initial implementation.
 *
 * \see Consistent Overhead Byte Stuffing - Reduced (COBS/R)
 *      Craig McQueen, 2010
 *      https://pythonhosted.org/cobs/cobsr-intro.html
 *      https://bitbucket.org/cmcqueen1975/cobs-python
 *
 * \see Consistent Overhead Byte Stuffing
 *      Stuart Cheshire and Mary Baker
 *      IEEE/ACM Transations on Networking, Vol. 7, No. 2, April 1999
 *      http://www.stuartcheshire.org/papers/COBSforToN.pdf
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "cobsr_codec.h"
#include "errors.h"

#include <limits.h>
#include <stdbool.h>

#include <stdint.h>
#include <unistd.h>


#define COBSR_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)   ((SRC_LEN) + ((SRC_LEN)/254u) + 1)
#define COBSR_DECODE_DST_BUF_LEN_MAX(SRC_LEN)   (SRC_LEN)


// Same structure for encoder and decoder
// TODO: Replace in/out by ringbuffer
//
struct cobsr_codec_state {
    const uint8_t *in;
    ssize_t in_remain;
    uint8_t *out;
    ssize_t out_remain;

    uint8_t state;
    uint8_t code;
};


static inline void cobsr_decode_emit(struct cobsr_codec_state *s, uint8_t c)
{
    if (s->out_remain > 0)
        *s->out++ = c;
    s->out_remain--;
}


/**
 * Decode COBS/R encoded data.
 *
 * Output must be in one piece, input may be segmented
 *
 * \return   0 packet not yet complete
 *           1 packet complete
 *          -1 packet complete, output buffer overrun
 */
static int cobsr_decode(struct cobsr_codec_state *s)
{
    while (s->in_remain) {
        // Consume one input byte per loop
        //
        uint8_t c = *s->in++;
        s->in_remain--;

        if (s->state == 1) {
            // End of run. Append a 0 byte
            // - except for maximum length runs
            // - except for end of packet
            //
            if (s->code != 255 && c != 0)
                cobsr_decode_emit(s, 0);

            s->state = 0;
        }

        if (c == 0) {
            if (s->state) {
                // COBS/R special case
                //
                cobsr_decode_emit(s, s->code);
                s->state = 0;
            }

            return (s->out_remain >= 0) ? 1 : -1;
        }

        if (s->state > 1) {
            // Copy byte
            //
            cobsr_decode_emit(s, c);
            s->state--;
        }

        if (s->state == 0) {
            // Read next code byte
            //
            s->code = c;
            s->state = c;
        }
    }

    return 0;
}


/**
 * Encode data with COBS/R.
 *
 * Input must be in one piece, output may be segmented
 *
 * \return 0 packet not yet complete
 *         1 packet complete
 */
static int cobsr_encode(struct cobsr_codec_state *s)
{
    while (s->out_remain) {
        // Generate (at most) one output byte per loop
        //
        if (s->state == 0) {
            int n = 0;
            while (s->in[n] && n < 254 && n < s->in_remain)
                n++;

            if (n && n == s->in_remain && n < s->in[n-1]) {
                s->code = s->in[n-1];
                s->state = n;
                s->in_remain--;
            }
            else {
                s->code = n+1;
                s->state = n+1;
            }

            *s->out++ = s->code;
            s->out_remain--;
        }
        else {
            if (s->in_remain == 0) {
                *s->out++ = 0;
                s->out_remain--;
                return 1;
            }

            if (s->state > 1) {
                *s->out++ = *s->in++;
                s->out_remain--;
                s->in_remain--;
            }
            else if (s->code != 255) {
                s->in++;
                s->in_remain--;
            }
            s->state--;
        }
    }

    return 0;
}



int cobsr_encode_rb(struct ringbuf *rb, const void *data, size_t len)
{
    if (rb_bytes_free(rb) < COBSR_ENCODE_DST_BUF_LEN_MAX(len) + 1) {
        errno = EWOULDBLOCK;
        return -1;
    }

    struct cobsr_codec_state encoder = {
        .in = data, .in_remain = len
    };

    void *ptr1, *ptr2;
    size_t len1, len2;

    rb_get_pointers(rb, RB_WRITE, INT_MAX, &ptr1, &len1, &ptr2, &len2);

    encoder.out = ptr1;
    encoder.out_remain = len1;

    cobsr_encode(&encoder);
    size_t done = len1 - encoder.out_remain;

    if (encoder.in_remain) {
        encoder.out = ptr2;
        encoder.out_remain = len2;
        cobsr_encode(&encoder);
        done += len2 - encoder.out_remain;
    }

    rb_commit(rb, RB_WRITE, done);

    return len;
}


struct cobsr_codec_state decoder;


int cobsr_decode_rb(struct ringbuf *rb, void *data, size_t len)
{
    void *ptr1, *ptr2;
    size_t len1, len2;

    rb_get_pointers(rb, RB_READ, INT_MAX, &ptr1, &len1, &ptr2, &len2);

    decoder.in = ptr1;
    decoder.in_remain = len1;
    decoder.out = data;
    decoder.out_remain = len;

    int ret = cobsr_decode(&decoder);
    size_t done = len1 - decoder.in_remain;

    if (ret == 0) {
        decoder.in = ptr2;
        decoder.in_remain = len2;
        ret = cobsr_decode(&decoder);
        done += len2 - decoder.in_remain;
    }

    rb_commit(rb, RB_READ, done);

    if (ret > 0) {
        return done;
    }
    else if (ret < 0) {
        errno = EMSG_TOO_LONG;
        return -1;
    }
    else {
        return 0;
    }
}

