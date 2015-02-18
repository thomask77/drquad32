/**
 * \file crc32_sm.c
 * Functions and types for CRC checks.
 *
 * Generated on Fri Jul 18 14:20:45 2014,
 * by pycrc v0.8.1, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 32
 *    Poly         = 0x04c11db7
 *    XorIn        = 0xffffffff
 *    ReflectIn    = True
 *    XorOut       = 0xffffffff
 *    ReflectOut   = True
 *    Algorithm    = table-driven
 *****************************************************************************/
#include "crc32_sm.h"     /* include the header file generated with pycrc */
#include <stdlib.h>
#include <stdint.h>

/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const crc32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

/**
 * Reflect all bits of a \a data word of \a data_len bytes.
 *
 * \param data         The data word to be reflected.
 * \param data_len     The width of \a data expressed in number of bits.
 * \return             The reflected data.
 *****************************************************************************/
crc32_t crc32_reflect(crc32_t data, size_t data_len)
{
    unsigned int i;
    crc32_t ret;

    ret = data & 0x01;
    for (i = 1; i < data_len; i++) {
        data >>= 1;
        ret = (ret << 1) | (data & 0x01);
    }
    return ret;
}


/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
crc32_t crc32_update(crc32_t crc, const unsigned char *data, size_t data_len)
{
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = crc ^ (*data >> (0 * 4));
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4);
        tbl_idx = crc ^ (*data >> (1 * 4));
        crc = crc_table[tbl_idx & 0x0f] ^ (crc >> 4);

        data++;
    }
    return crc & 0xffffffff;
}

