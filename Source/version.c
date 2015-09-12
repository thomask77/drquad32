#include "version.h"
#include "ansi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "util.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>


const struct version_info version_info = {
    .vcs_info_start = VCS_INFO_START,
    .product_name   = "DrMOS Quad 32",
    .major          = 0,
    .minor          = 9,
    .patch          = 0,
    .vcs_info_end   = VCS_INFO_END
};


const struct version_info *find_version_info(const void *buf, size_t size)
{
    for (size_t i = 0; i < size - sizeof(struct version_info); i += 4) {
        const struct version_info *info =
                (const struct version_info *)((uint32_t)buf + i);

        if (!memcmp(info->vcs_info_start, VCS_INFO_START, 16) &&
            !memcmp(info->vcs_info_end,   VCS_INFO_END,   16)  )
            return info;
    }

    return NULL;
}


void print_version_info(const struct version_info *info, int verbose)
{
    printf(
        ANSI_FG_LTGREEN "%s" ANSI_NORMAL " v%d.%d.%d %s %s %s\n"
        "Copyright (c)2015 Thomas Kindler <mail@t-kindler.de>\n\n",
        version_info.product_name,
        version_info.major,
        version_info.minor,
        version_info.patch,
        version_info.vcs_id,
        version_info.build_date, version_info.build_time
    );

    if (verbose > 0) {
        printf(
            "  Compiled  %s %s\n"
            "            by %s on %s\n"
            "  Image     %08lx, %lu bytes\n",
            version_info.build_date, version_info.build_time,
            version_info.build_user, version_info.build_host,
            version_info.image_start, version_info.image_size
        );
    }
}


// -------------------- Shell commands --------------------
//
#include "command.h"

// Unique device ID register (96 bits)
//
#define U_ID    ((__IO uint32_t *)0x1FFF7A10)


void cmd_about(void)
{
    print_version_info(&version_info, 1);

    printf(
        "  GCC       %s\n"
        "  newlib    %s\n"
        "  FreeRTOS  %s\n"
        "  FatFs     %d\n"
        "  CMSIS     %x.%x\n"
        "  StdPeriph %x.%x.%x\n"
        "  CPU ID    %08lx %08lx %08lx\n",
        __VERSION__,
        _NEWLIB_VERSION,
        tskKERNEL_VERSION_NUMBER,
        _FATFS,
        __CM4_CMSIS_VERSION_MAIN,
        __CM4_CMSIS_VERSION_SUB,
        __STM32F4XX_STDPERIPH_VERSION_MAIN,
        __STM32F4XX_STDPERIPH_VERSION_SUB1,
        __STM32F4XX_STDPERIPH_VERSION_SUB2,
        U_ID[0], U_ID[1], U_ID[2]
    );
}

SHELL_CMD(about, (cmdfunc_t)cmd_about, "about")
