/**
 * State-machine based COBS/R codec for communication packet framing.
 *
 * Copyright (c)2015 Thomas Kindler <mail_cobsr@t-kindler.de>
 *
 * 2015-11-28: tk, new implementation for segmented input/output buffers.
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

#ifndef COBSR_CODEC_H
#define COBSR_CODEC_H


// Maximum buffer lengths including end-of-packet zero byte
//
#define COBSR_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)   ((SRC_LEN) + ((SRC_LEN)/254u) + 1 + 1)
#define COBSR_DECODE_DST_BUF_LEN_MAX(SRC_LEN)   (SRC_LEN + 1)


struct cobsr_decoder_state {
    const char  *in,  *in_end;
    char        *out, *out_end;

    unsigned char   code, run;
};


/**
 * Decode COBS/R encoded data.
 *
 * Output must be in one piece, input may be segmented
 *
 * \return  0 input buffer empty, call again with new buffer
 *          1 packet complete
 */
static inline int cobsr_decode(struct cobsr_decoder_state *s)
{
    while (s->in != s->in_end) {
        char c = *s->in++;

        if (s->out == s->out_end) {
            // Output buffer full, wait for end-of-packet
            //
            if (c == 0) {
                s->run = 0;
                return 1;
            }
        }
        else if (c == 0) {
            if (s->run > 1) {
                // COBS/R special case
                //
                *s->out++ = s->code;
            }
            s->run = 0;
            return 1;
        }
        else if (s->run > 1) {
            // Copy byte
            //
            *s->out++ = c;
            s->run--;
        }
        else {
            if (s->run == 1 && s->code != 255) {
                // Append 0 except for long runs
                //
                *s->out++ = 0;
            }
            s->run = c;
            s->code = c;
        }
    }

    return 0;
}


struct cobsr_encoder_state {
    const char  *in,  *in_end;
    char        *out, *out_end;

    enum {
        COBSR_NORMAL,
        COBSR_LONG,
        COBSR_SPECIAL
    } mode;

    unsigned char   run;
};


/**
 * Encode data with COBS/R.
 *
 * Input must be in one piece, output may be segmented
 *
 * \return  0 output buffer full, call again with new buffer
 *          1 packet complete
 */
static inline int cobsr_encode(struct cobsr_encoder_state *s)
{
    while (s->out != s->out_end) {
        if (s->run > 1) {
            // Copy byte
            //
            *s->out++ = *s->in++;
            s->run--;
        }
        else if (s->run == 1) {
            // End of run
            //
            s->run = 0;

            if (s->mode == COBSR_SPECIAL)
                s->in++;

            if (s->in == s->in_end) {
                // Append terminating zero
                //
                *s->out++ = 0;
                return 1;
            }

            if (s->mode == COBSR_NORMAL) {
                // Skip 0 except for long runs
                //
                s->in++;
            }
        }
        else {
            // Look-Ahead
            //
            while (s->run < 254  &&  s->in + s->run != s->in_end  &&  s->in[s->run])
                s->run++;

            if (s->run  &&  s->in + s->run == s->in_end  &&  s->run < s->in_end[-1]) {
                s->mode = COBSR_SPECIAL;
                *s->out++ = s->in_end[-1];
            }
            else {
                s->mode = (s->run == 254) ? COBSR_LONG : COBSR_NORMAL;
                *s->out++ = ++s->run;
            }
        }
    }

    return 0;
}


#endif
