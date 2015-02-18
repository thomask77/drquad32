/**
 * Record file system for the flash OTP area
 *
 * Copyright (c)2001-2015 Thomas Kindler <mail@t-kindler.de>
 *
 * 2014-03-17: tk, initial implementation
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
#include "otp_record.h"
#include "util.h"
#include "stm32f4xx.h"
#include <string.h>


#ifdef STM32F40_41xxx

#define OTP_BASE    0x1FFF7800
#define OTP_SIZE    512

#endif


int otp_read_rec(int type, void *buf, size_t size)
{
    int ret = -1;
    const uint8_t *p   = (uint8_t*)OTP_BASE;
    const uint8_t *end = p + OTP_SIZE;

    while (p < end-1) {
        uint8_t t = *p++;

        // Skip invalidated records
        //
        if (t == OTP_TYPE_ZERO)
            continue;

        // End marker
        //
        if (t == OTP_TYPE_END)
            break;

        uint8_t s = *p++;

        if (t == type) {
            ret = size < s ? size : s;
            memcpy(buf, p, ret);
        }

        p += s;
    }

    return ret;
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "lwip/stats.h"
#include <string.h>


static void cmd_otp_write(int argc, char *argv[])
{
    if (argc < 3)
        goto usage;

    char *endp;

    int len = argc - 2;
    uint32_t addr = strtoul(argv[1], &endp, 0);
    if (*endp)
        goto usage;

    if (addr < 0 || addr + len > 511)
        goto usage;

    uint8_t *data = alloca(len);
    for (int i=0; i<len; i++) {
        data[i] = strtoul(argv[i+2], &endp, 0);
        if (*endp)
            goto usage;
    }

    printf("Are you sure?\n");
    if (getchar() != 'y')
        return;

    printf("Really?\n");
    if (getchar() != 'y')
        return;

    FLASH_Unlock();

    for (int i=0; i<len; i++)
        FLASH_ProgramByte(OTP_BASE + addr + i, data[i]);

    FLASH_Lock();
    return;

usage:
    printf("Usage: %s <address 0..511> <byte 0> [byte n..]\n", argv[0]);
}


static void cmd_otp_show(void)
{
    hexdump((void*)OTP_BASE, 512);

    const uint8_t *p   = (uint8_t*)OTP_BASE;
    const uint8_t *end = p + OTP_SIZE;

    printf("\n");

    int records = 0;
    while (p < end-1) {
        uint8_t t = *p++;

        // Skip invalidated records
        //
        if (t == OTP_TYPE_ZERO)
            continue;

        // End marker
        //
        if (t == OTP_TYPE_END)
            break;

        uint8_t s = *p++;

        printf("type=0x%02x, size=%d:", t, s);
        for (int i=0; i<s; i++)
            printf(" %02x", p[i]);

        printf("\n");

        p += s;
        records++;
    }

    printf("%d records.\n", records);
}


SHELL_CMD(otp_write, (cmdfunc_t)cmd_otp_write, "write to OTP area")
SHELL_CMD(otp_show,  (cmdfunc_t)cmd_otp_show,  "show OTP data")
