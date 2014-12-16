/**
 * Realtime clock driver
 *
 * Copyright (c)2009 Thomas Kindler <mail@t-kindler.de>
 *
 * This program is free software;  you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License,  or (at your option) any later version.  Read the
 * full License at http://www.gnu.org/copyleft for more details.
 */

// Include files -----
//
#include "rtc.h"
#include "stm32f10x.h"
#include <time.h>
#include <FreeRTOS.h>


int _gettimeofday(struct timeval *tp, void *tzvp)
{
    if (tp) {
        unsigned divh, divl, cnth, cntl;

        vPortEnterCritical();
        do {
            divh = RTC->DIVH & 0x0F;
            divl = RTC->DIVL;
            cnth = RTC->CNTH;
            cntl = RTC->CNTL;
        } while (divh != (RTC->DIVH & 0x0F));
        vPortExitCritical();

        tp->tv_sec  = (cnth << 16) + cntl;
        tp->tv_usec = RTC_JIFFIES_TO_US( RTC_PRESCALER - (divh << 16) - divl );
    }

    struct timezone *tzp = tzvp;
    if (tzp)  {
        tzp->tz_minuteswest = 0;
        tzp->tz_dsttime = 0;
    }

    return 0;
}


int _settimeofday(const struct timeval *tp, const struct timezone *tzvp)
{
    RTC_SetCounter(tp->tv_sec);
    return 0;
}


uint32_t rtc_get_jiffies(void)
{
    vPortEnterCritical();

    static unsigned last, count;

    unsigned  divh, divl;
    do {
        divh = RTC->DIVH & 0x0F;
        divl = RTC->DIVL;
    } while (divh != (RTC->DIVH & 0x0F));

    unsigned ticks = RTC_PRESCALER - (divh << 16) - divl;

    count += ticks - last;

    if (ticks < last)
        count += RTC_PRESCALER;

    last = ticks;

    vPortExitCritical();

    return count;
}


void rtc_init(void)
{
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_PWR |
        RCC_APB1Periph_BKP,
        ENABLE
    );

    PWR_BackupAccessCmd(ENABLE);
    // BKP_DeInit();

    RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);
    RCC_RTCCLKCmd(ENABLE);

    RTC_WaitForSynchro();
    RTC_WaitForLastTask();

    RTC_SetPrescaler(RTC_PRESCALER-1);
    RTC_WaitForLastTask();
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include <stdlib.h>
#include <stdio.h>

void cmd_date(int argc, char *argv[])
{
    if (argc == 1) {
        time_t now    = time(NULL);
        struct tm *tm = localtime(&now);

        printf("%02d:%02d:%02d, %02d.%02d.%04d\n",
            tm->tm_hour, tm->tm_min,   tm->tm_sec,
            tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900
        );
    }
    else if (argc == 2) {
        struct timeval tv;
        tv.tv_sec = atoi(argv[1]);
        if (tv.tv_sec == 0)
            goto usage;

        _settimeofday(&tv, NULL);
    }
    else {
usage:
        printf("usage: %s [timestamp]\n", argv[0]);
    }
}

SHELL_CMD(date, (cmdfunc_t)cmd_date, "get/set system time");
