#include "version.h"
#include "git_version.h"
#include "ansi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "util.h"
#include "ff.h"
#include <stdio.h>

#define PRODUCT_NAME    "DrMOS Quad 32"
#define VERSION_MAJOR   0
#define VERSION_MINOR   9
#define VERSION_PATCH   0

// -----

// TODO: Per Linkerscript ganz an den Anfang stellen
//
const struct version_info version_info = {
    .product_name = PRODUCT_NAME,
    .major        = VERSION_MAJOR,
    .minor        = VERSION_MINOR,
    .patch        = VERSION_PATCH,
    .string       = "v" STRINGIFY(VERSION_MAJOR) "."
                        STRINGIFY(VERSION_MINOR) "."
                        STRINGIFY(VERSION_PATCH),
    .git_version  = GIT_VERSION,
    .build_user   = BUILD_USER,
    .build_host   = BUILD_HOST,
    .build_date   = BUILD_DATE,
    .build_time   = BUILD_TIME
};


// Unique device ID register (96 bits)
//
#define U_ID    ((__IO uint32_t *)0x1FFF7A10)


void print_version_info(int verbose)
{
    printf(
        ANSI_FG_LTGREEN "%s" ANSI_NORMAL " %s %s %s %s\n"
        "Copyright (c)2014 Thomas Kindler <mail@t-kindler.de>\n"
        "\n", 
        version_info.product_name, version_info.string, 
        version_info.git_version, version_info.build_date,
        version_info.build_time
    );

    if (verbose) {
        printf(
            "  Compiled  %s %s\n"
            "            by %s on %s\n"
            "  GCC       %s\n"
            "  newlib    %s\n"
            "  FreeRTOS  %s\n"
            "  FatFs     %d\n"
            "  CMSIS     %x.%x\n"
            "  StdPeriph %x.%x.%x\n"
            "  CPU ID    %08lx %08lx %08lx\n",
            version_info.build_date, version_info.build_time,
            version_info.build_user, version_info.build_host,
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
}


// -------------------- Shell commands --------------------
//
#include "command.h"

void cmd_about(void)
{
    print_version_info(1);
}

SHELL_CMD(about, (cmdfunc_t)cmd_about, "about")
