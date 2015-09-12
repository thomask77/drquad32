/**
 * Utility functions
 *
 * Copyright (c) Thomas Kindler <mail@t-kindler.de>
 *
 * 2013-03-25: tk, GPIO test commands
 * 2013-03-25: tk, refactored parse_range from parameter.c
 * 2009-05-14: tk, added some shell commands
 * 2009-04-22: tk, added crc32 and statistics function
 * 2005-11-30: tk, improved hexdump with ascii output
 * 2002-01-10: tk, added strnbar function for progress bars
 * 2000-04-24: tk, initial implementation
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

#include "util.h"
#include "command.h"
#include "FreeRTOS.h"
#include "task.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

/**
 * Parse a range-specifier string
 *
 * \param arg       string to parse
 * \param first     pointer to store the first value
 * \param last      pointer to store the last value
 * \return          1 if string was valid, 0 otherwise
 *
 * \note  The following specifiers are supported:
 *   "n"    -> first = n, last = n
 *   "x:y"  -> first = x, last = y
 *   "x:"   -> first = x, last = INT_MAX
 *   ":y"   -> first = INT_MIN, last = y
 *   ":"    -> first = INT_MIN, last = INT_MAX
 */
int parse_range(const char *arg, int *first, int *last)
{
    char *s  = strcpy( alloca( strlen(arg)+1 ), arg );
    char *s2 = strchr(s, ':'), *endp;

    *first = INT_MIN;
    *last  = INT_MAX;

    if (s2) {
        // number range
        *s2++ = '\0';
        if (*s) {
            *first = strtol(s, &endp, 0);
            if (*endp) return 0;
        }
        if (*s2) {
            *last = strtol(s2, &endp, 0);
            if (*endp) return 0;
        }
    }
    else {
        // single number
        *first = *last = strtol(s, &endp, 0);
        if (*endp) return 0;
    }

    return 1;
}


/**
 * Make a text-mode bargraph string.
 *
 * \param  s      pointer to string buffer
 * \param  len    length of string buffer
 * \param  value  bargraph value
 * \param  min    left edge of bargraph range
 * \param  max    right edge of bargraph range
 *
 * \return  pointer to the string buffer
 */
char *strnbar(char *s, int len, float value, float min, float max)
{
    const int n = floorf( ((value - min) * len) / (max-min) );

    for (int i=0; i<len-1; i++)
        s[i] = i < n ? '=' : ' ';

    if (n < 0)
        s[0] = '<';
    else if (n > len-1)
        s[len-2] = '>';

    s[len-1] = '\0';

    return s;
}


/**
 * Generate a nice hexdump of a memory area.
 *
 * \param  mem     pointer to memory to dump
 * \param  length  how many bytes to dump
 */
void hexdump(const void *mem, unsigned length)
{
    puts("       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    0123456789ABCDEF");

    const unsigned char *src = (const unsigned char*)mem;
    for (unsigned i=0; i<length; i += 16, src += 16) {
        char  line[80], *t = line;

        t += sprintf(t, "%04x:  ", i);
        for (unsigned j=0; j<16; j++) {
            if (i+j < length)
                t += sprintf(t, "%02X", src[j]);
            else
                t += sprintf(t, "  ");
            *t++ = j & 1 ? ' ' : '-';
        }

        t += sprintf(t, "  ");
        for (unsigned j=0; j<16; j++) {
            if (i+j < length)
                *t++ = isprint(src[j]) ? src[j] : '.';
            else
                *t++ = ' ';
        }
        *t++ = 0;
        puts(line);
    }
}


/**
 * Update statistics.
 *
 * \see  http://www.dspguide.com/ch2/2.htm
 *       for implementation details.
 *
 * \param  s    pointer to stats structure
 * \param  val  value to process
 */
void stats_update(struct stats *s, float val)
{
    s->n++;
    s->sum   += val;
    s->sqsum += val * val;

    if (val > s->max)  s->max = val;
    if (val < s->min)  s->min = val;

    s->mean = s->sum / s->n;
    s->var  = (s->sqsum - (s->sum * s->sum) / s->n) / (s->n-1);
    if (s->var < 0)
        s->var = 0;
    s->std  = sqrtf(s->var);
}


/**
 * Reset statistics.
 *
 * \param  s    pointer to stats structure
 */
void stats_reset(struct stats *s)
{
    *s = (struct stats) {
        .min  =  INFINITY,
        .max  = -INFINITY
    };
}


/**
 * Convert HSV to RGB colors.
 *
 * \param  hsv  hue, saturation and value (0x00HHSSVV)
 * \return rgb  value (0x00RRGGBB)
 *
 */
uint32_t hsv_to_rgb(uint32_t hsv)
{
    int h = ((hsv >> 16) & 255) * 6;
    int s =  (hsv >>  8) & 255;
    int v =  (hsv >>  0) & 255;

    if (s == 0)
        return  (v<<16) | (v<<8) | (v<<0);

    int f = h & 255;    // fractional part of h
    int p = v * (255 - s) / 256;
    int q = v * (255 - (s * f) / 256) / 256;
    int t = v * (255 - (s * (255 - f)) / 256) / 256;

    switch (h / 256) {
    case 0:  return  (v<<16) | (t<<8) | (p<<0);
    case 1:  return  (q<<16) | (v<<8) | (p<<0);
    case 2:  return  (p<<16) | (v<<8) | (t<<0);
    case 3:  return  (p<<16) | (q<<8) | (v<<0);
    case 4:  return  (t<<16) | (p<<8) | (v<<0);
    case 5:  return  (v<<16) | (p<<8) | (q<<0);
    default: return 0;  // should not be possible
    }
}


// -------------------- Shell commands --------------------
//
#include "ansi.h"
#include "syscalls.h"

/**
 * Execute a command periodically.
 *
 * \todo  make the delay interruptible
 * \todo  use system() instead of exec()
 */
static void cmd_watch(int argc, char *argv[])
{
    int interval = 100;
    if (argc < 2)
        goto usage;

    if (!strcmp(argv[1], "-n")) {
        if (argc < 4)
            goto usage;
        interval = atoi(argv[2]);
        if (interval < 0)
            goto usage;

        argc -= 2;
        argv += 2;
    }

    for (;;) {
        printf(ANSI_CLEAR ANSI_HOME "every %dms:", interval);
        for (int i=1; i<argc; i++)
            printf(" %s", argv[i]);
        printf("\n\n");

        cmd_exec(argc - 1, &argv[1]);
        fflush(stdout);

        int delay = interval;
        do {
            while (stdin_chars_avail()) {
                if (getchar() == 3) // CTRL-C
                    return;
            }

            vTaskDelay(delay > 100 ? 100 : delay);
            delay -= 100;
        } while (delay > 0);
    }

    return;

usage:
    printf("usage: %s [-n <ms>] <command>\n", argv[0]);
}


void cmd_dump(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
        goto usage;

    char     *endp;
    void     *addr = (void*)strtoul(argv[1], &endp, 0);
    unsigned  len  = 256;

    if (*endp)
        goto usage;

    if (argc == 3) {
        len = strtoul(argv[2], &endp, 0);
        if (*endp)
            goto usage;
    }

    hexdump(addr, len);
    return;

usage:
    printf("Usage: %s <addr> [len]\n", argv[0]);
}

/**
 * Show FreeRTOS runtime statistics.
 */
static void cmd_ps(void)
{
    int n = uxTaskGetNumberOfTasks();
    TaskStatus_t *stats = malloc(n * sizeof(TaskStatus_t));

    if (!stats) {
        printf("can't alloc buffer\n");
        return;
    }

    uint32_t  total_run_time;
    n = uxTaskGetSystemState(stats, n, &total_run_time);

    printf(" # Pri State Name             CPU Time   %%   Stack\n");

    for (int i=0; i<n; i++) {
        char status;
        switch(stats[i].eCurrentState) {
        case eReady:     status = 'R'; break;
        case eBlocked:   status = 'B'; break;
        case eSuspended: status = 'S'; break;
        case eDeleted:   status = 'D'; break;
        default:         status = '?'; break;
        }

        printf("%2lu %3lu %-5c %-16s %10lu %3lu %5d\n",
            stats[i].xTaskNumber, stats[i].uxCurrentPriority,
            status, stats[i].pcTaskName, stats[i].ulRunTimeCounter,
            (uint32_t)(stats[i].ulRunTimeCounter * 100ULL / total_run_time),
            stats[i].usStackHighWaterMark
        );
    }

    free(stats);
}

static void cmd_sysinfo(void)
{
    extern char _etext;    // end address of the .text section
    extern char _sidata;   // start address of the initialization values of the .data section
    extern char _sdata;    // start address of the .data section
    extern char _edata;    // end address of the .data section
    extern char _sbss;     // start address of the .bss section
    extern char _ebss;     // end address of the .bss section
    extern char _snoinit;  // start address of the .noinit section
    extern char _enoinit;  // end address of the .noinit section
    extern char _end;      // end address of the .bss section
    extern char _estack;   // initial value of the stack pointer

    extern char *__brkval;

    printf("RCC->CSR = 0x%08x\n", (unsigned int)RCC->CSR);
    printf("_etext   = %p\n", &_etext  );
    printf("_sidata  = %p\n", &_sidata );
    printf("_sdata   = %p\n", &_sdata  );
    printf("_edata   = %p\n", &_edata  );
    printf("_sbss    = %p\n", &_sbss   );
    printf("_ebss    = %p\n", &_ebss   );
    printf("_snoinit = %p\n", &_snoinit);
    printf("_enoinit = %p\n", &_enoinit);
    printf("_end     = %p\n", &_end    );
    printf("_estack  = %p\n", &_estack );

    printf("heap avail       = %10d\n", (char*)__get_MSP() - __brkval);
    malloc_stats();

    RCC_ClocksTypeDef  RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    printf("SYSCLK_Frequency = %10ld\n", RCC_ClocksStatus.SYSCLK_Frequency );
    printf("HCLK_Frequency   = %10ld\n", RCC_ClocksStatus.HCLK_Frequency   );
    printf("PCLK1_Frequency  = %10ld\n", RCC_ClocksStatus.PCLK1_Frequency  );
    printf("PCLK2_Frequency  = %10ld\n", RCC_ClocksStatus.PCLK2_Frequency  );
}


static void cmd_reset(void)
{
    NVIC_SystemReset();
}


static GPIO_TypeDef *GPIOS[] = {
    GPIOA, GPIOB, GPIOC, GPIOD,
    GPIOE, GPIOF, GPIOG, GPIOH
};


static void cmd_gpio_show(void)
{
    const int N = ARRAY_SIZE(GPIOS);

    uint16_t idr[N];

    for (int i=0; i<N; i++)
        idr[i] = GPIOS[i]->IDR;

    printf("    ");
    for (int i=0; i<N; i++)
        printf("GPIO%c   ", 'A' + i);
    printf("\n");

    printf("    ");
    for (int i=0; i<N; i++)
        printf("0x%04x  ", idr[i]);
    printf("\n");

    for (int bit=0; bit<16; bit++) {
        printf("%2d  ", bit);
        for (int i=0; i<N; i++)
            printf("%d       ", idr[i] & (1<<bit) ? 1 : 0);
        printf("\n");
    }
}


static void cmd_gpio_set(int argc, char *argv[])
{
    if (argc < 3 || strlen(argv[1]) < 2)
        goto usage;

    int g = argv[1][0] - 'A';
    int b = atoi(&argv[1][1]);
    int o = atoi(argv[2]);

    if (g < 0 || g >= ARRAY_SIZE(GPIOS) || b < 0 || b > 15 || o < 0 || o > 1)
        goto usage;

    GPIO_WriteBit(GPIOS[g], (1<<b), o);

    GPIO_Init(GPIOS[g], &(GPIO_InitTypeDef) {
        .GPIO_Pin = 1 << b,
        .GPIO_Mode = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd = GPIO_PuPd_NOPULL
    } );

    return;

usage:
    printf("Usage: %s <[A-K][0-15]> <0,1>\n", argv[0]);
}


SHELL_CMD(watch,     (cmdfunc_t)cmd_watch,      "execute periodically")
SHELL_CMD(dump,      (cmdfunc_t)cmd_dump,       "dump memory area")
SHELL_CMD(sysinfo,   (cmdfunc_t)cmd_sysinfo,    "show system information")
SHELL_CMD(ps,        (cmdfunc_t)cmd_ps,         "show tasks")
SHELL_CMD(reset,     (cmdfunc_t)cmd_reset,      "system reset")
SHELL_CMD(gpio_show, (cmdfunc_t)cmd_gpio_show,  "show GPIO state")
SHELL_CMD(gpio_set,  (cmdfunc_t)cmd_gpio_set,   "set GPIO state")
