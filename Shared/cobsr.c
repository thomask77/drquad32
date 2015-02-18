/**
 * Consistent Overhead Byte Stuffing--Reduced (COBS/R)
 *
 * Copyright (c) 2010 Craig McQueen
 * Copyright (c) 2015 Thomas Kindler <mail@t-kindler.de>
 *
 * 2015-02-18, tk:  Functions now use errno and return -1 on failure.
 *                  Use void pointers for source and destination.
 *                  Assert on NULL-Pointers (it's a programming error!).
 *                  Integrated license text into the source file.
 *
 * 2014-07-11, tk:  Downloaded from https://bitbucket.org/cmcqueen1975/cobs-c
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "cobsr.h"
#include "errors.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>

/*****************************************************************************
 * Functions
 ****************************************************************************/

ssize_t cobsr_encode(void *dst_buf_ptr_v, size_t dst_buf_len, const void *src_ptr_v, size_t src_len)
{
    uint8_t *dst_buf_ptr = dst_buf_ptr_v;
    const uint8_t *src_ptr = src_ptr_v;

    const uint8_t *     src_end_ptr         = src_ptr + src_len;
    uint8_t *           dst_buf_end_ptr     = dst_buf_ptr + dst_buf_len;
    uint8_t *           dst_code_write_ptr  = dst_buf_ptr;
    uint8_t *           dst_write_ptr       = dst_code_write_ptr + 1;
    uint8_t             src_byte            = 0;
    uint8_t             search_len          = 1;

    assert(dst_buf_ptr  && src_ptr);

    if (src_len != 0)
    {
        /* Iterate over the source bytes */
        for (;;)
        {
            /* Check for running out of output buffer space */
            if (dst_write_ptr >= dst_buf_end_ptr)
            {
                errno = ECOBSR_ENCODE_OUT_BUFFER_OVERFLOW;
                return -1;
            }

            src_byte = *src_ptr++;
            if (src_byte == 0)
            {
                /* We found a zero byte */
                *dst_code_write_ptr = search_len;
                dst_code_write_ptr = dst_write_ptr++;
                search_len = 1;
                if (src_ptr >= src_end_ptr)
                {
                    break;
                }
            }
            else
            {
                /* Copy the non-zero byte to the destination buffer */
                *dst_write_ptr++ = src_byte;
                search_len++;
                if (src_ptr >= src_end_ptr)
                {
                    break;
                }
                if (search_len == 0xFF)
                {
                    /* We have a long string of non-zero bytes, so we need
                     * to write out a length code of 0xFF. */
                    *dst_code_write_ptr = search_len;
                    dst_code_write_ptr = dst_write_ptr++;
                    search_len = 1;
                }
            }
        }
    }

    /* We've reached the end of the source data (or possibly run out of output buffer)
     * Finalise the remaining output. In particular, write the code (length) byte.
     *
     * For COBS/R, the final code (length) byte is special: if the final data byte is
     * greater than or equal to what would normally be the final code (length) byte,
     * then replace the final code byte with the final data byte, and remove the final
     * data byte from the end of the sequence. This saves one byte in the output.
     *
     * Update the pointer to calculate the final output length.
     */
    if (dst_code_write_ptr >= dst_buf_end_ptr)
    {
        /* We've run out of output buffer to write the code byte. */
        errno = ECOBSR_ENCODE_OUT_BUFFER_OVERFLOW;
        return -1;
    }
    else
    {
        if (src_byte < search_len)
        {
            /* Encoding same as plain COBS */
            *dst_code_write_ptr = search_len;
        }
        else
        {
            /* Special COBS/R encoding: length code is final byte,
             * and final byte is removed from data sequence. */
            *dst_code_write_ptr = src_byte;
            dst_write_ptr--;
        }
    }

    /* Calculate the output length, from the value of dst_code_write_ptr */
    return dst_write_ptr - dst_buf_ptr;
}


ssize_t cobsr_decode(void *dst_buf_ptr_v, size_t dst_buf_len, const void *src_ptr_v, size_t src_len)
{
    uint8_t *dst_buf_ptr = dst_buf_ptr_v;
    const uint8_t *src_ptr = src_ptr_v;

    const uint8_t *     src_end_ptr         = src_ptr + src_len;
    uint8_t *           dst_buf_end_ptr     = dst_buf_ptr + dst_buf_len;
    uint8_t *           dst_write_ptr       = dst_buf_ptr;
    size_t              remaining_input_bytes;
    size_t              remaining_output_bytes;
    size_t              num_output_bytes;
    uint8_t             src_byte;
    uint8_t             i;
    uint8_t             len_code;


    assert(dst_buf_ptr  && src_ptr);

    if (src_len != 0)
    {
        for (;;)
        {
            len_code = *src_ptr++;
            if (len_code == 0)
            {
                errno = ECOBSR_DECODE_ZERO_BYTE_IN_INPUT;
                return -1;
            }

            /* Calculate remaining input bytes */
            remaining_input_bytes = src_end_ptr - src_ptr;

            if ((len_code - 1) < (int)remaining_input_bytes)
            {
                num_output_bytes = len_code - 1;

                /* Check length code against remaining output buffer space */
                remaining_output_bytes = dst_buf_end_ptr - dst_write_ptr;
                if (num_output_bytes > remaining_output_bytes)
                {
                    errno = ECOBSR_DECODE_OUT_BUFFER_OVERFLOW;
                    return -1;
                }

                for (i = num_output_bytes; i != 0; i--)
                {
                    src_byte = *src_ptr++;
                    if (src_byte == 0)
                    {
                        errno = ECOBSR_DECODE_ZERO_BYTE_IN_INPUT;
                        return -1;
                    }
                    *dst_write_ptr++ = src_byte;
                }

                /* Add a zero to the end */
                if (len_code != 0xFF)
                {
                    if (dst_write_ptr >= dst_buf_end_ptr)
                    {
                        errno = ECOBSR_DECODE_OUT_BUFFER_OVERFLOW;
                        return -1;
                    }
                    *dst_write_ptr++ = 0;
                }
            }
            else
            {
                /* We've reached the last length code, so write the remaining
                 * bytes and then exit the loop. */

                num_output_bytes = remaining_input_bytes;

                /* Check length code against remaining output buffer space */
                remaining_output_bytes = dst_buf_end_ptr - dst_write_ptr;
                if (num_output_bytes > remaining_output_bytes)
                {
                    errno = ECOBSR_DECODE_OUT_BUFFER_OVERFLOW;
                    return -1;
                }

                for (i = num_output_bytes; i != 0; i--)
                {
                    src_byte = *src_ptr++;
                    if (src_byte == 0)
                    {
                        errno = ECOBSR_DECODE_ZERO_BYTE_IN_INPUT;
                        return -1;
                    }
                    *dst_write_ptr++ = src_byte;
                }

                /* Write final data byte, if applicable for COBS/R encoding. */
                if (len_code - 1 > (int)remaining_input_bytes)
                {
                    if (dst_write_ptr >= dst_buf_end_ptr)
                    {
                        errno = ECOBSR_DECODE_OUT_BUFFER_OVERFLOW;
                        return -1;
                    }
                    else
                    {
                        *dst_write_ptr++ = len_code;
                    }
                }

                /* Exit the loop */
                break;
            }
        }
    }

    return dst_write_ptr - dst_buf_ptr;
}
