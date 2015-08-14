#include "parameter.h"
#include "param_table.h"
#include "stm32f4xx.h"
#include "watchdog.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>


/**
 * Check parameter table for correct ID order
 *
 */
static void param_check_table(void)
{
    static int checked = 0;
    if (!checked) {
        for (int i=0; i<param_count-1; i++)
            assert(param_table[i].id < param_table[i+1].id);
        checked = 1;
    }
}


/**
 * Get param_info by id.
 *
 */
const struct param_info *param_get_info(int id)
{
    param_check_table();

    int lower = 0;
    int upper = param_count;

    while (lower < upper) {
        int index = (lower + upper) / 2;
        const struct param_info *p = &param_table[index];

        if (id < p->id)
            upper = index;
        else if (id > p->id)
            lower = index + 1;
        else
            return p;
    }
    return NULL;
}


/**
 * Get parameter value by id.
 *
 * \note  double is used here because it can represent
 *        the full range of float and int32_t values.
 */
param_error param_get(int id, double *value)
{
    const struct param_info *p = param_get_info(id);

    if (!p)
        return PERR_UNKNOWN_ID;

    if (p->type == PTYPE_FLOAT)
        *value = *p->flt.ptr;
    else
        *value = *p->i32.ptr;

    return PERR_OK;
}


/**
 * Set parameter value by id.
 *
 * \note  double is used here because it can represent
 *        the full range of float and int32_t values.
 */
param_error param_set(int id, double value)
{
    const struct param_info *p = param_get_info(id);

    if (!p)
        return PERR_UNKNOWN_ID;

    if (p->readonly)
        return PERR_READONLY;

    if (p->type == PTYPE_FLOAT) {
        if (value < p->flt.min || value > p->flt.max)
            return PERR_INVALID_VALUE;
        *p->flt.ptr = value;
    }
    else {
        if (value < p->i32.min || value > p->i32.max)
            return PERR_INVALID_VALUE;
        *p->i32.ptr = value;
    }
    return PERR_OK;
}


void param_set_defaults(void)
{
    for (int i=0; i<param_count; i++) {
        const struct param_info *p = &param_table[i];
        if (p->noinit)
            continue;

        if (p->type == PTYPE_FLOAT)
            param_set(p->id, p->flt.def);
        else
            param_set(p->id, p->i32.def);
    }
}


const char *param_strerr(param_error error)
{
    switch (error) {
    case PERR_OK            : return "PERR_OK";
    case PERR_UNKNOWN_ID    : return "PERR_UNKNOWN_ID";
    case PERR_READONLY      : return "PERR_READONLY";
    case PERR_INVALID_VALUE : return "PERR_INVALID_VALUE";
    case PERR_INVALID_FORMAT: return "PERR_INVALID_FORMAT";
    default:  return "Unknown error";
    }
}


// -------------------- Load & Save --------------------
//
#define PARAM_MAGIC     0x334D5250  // "PRM3"

#define PARAM_BASE0     (FLASH_BASE + 0x8000)
#define PARAM_BASE1     (FLASH_BASE + 0xC000)

#define PARAM_SECTOR0   FLASH_Sector_2
#define PARAM_SECTOR1   FLASH_Sector_3


void param_load(void)
{
    uint32_t magic = *(uint32_t*)(PARAM_BASE0 + 0);
    uint32_t size  = *(uint32_t*)(PARAM_BASE0 + 4);

    if (magic != PARAM_MAGIC) {
        printf("Parameter image not found.\n");
        return;
    }

    int count = size / 8;
    uint32_t addr = PARAM_BASE0 + 8;

    printf("Loading %d parameters..\n", count);

    for (int i=0; i<count; i++) {
        uint32_t  id    = *(uint32_t*)(addr + 0);

        param_error err = PERR_UNKNOWN_ID;
        const struct param_info *p = param_get_info(id);

        if (p != NULL) {
            if (p->type == PTYPE_FLOAT)
                err = param_set(p->id, *(float*)(addr + 4));
            else
                err = param_set(p->id, *(int32_t*)(addr + 4));
        }

        if (err != PERR_OK)
            printf("%lu: %s\n", id, param_strerr(err));

        addr += 8;
    }
}


void param_save(void)
{
    printf("Saving parameters.. ");

    // Erasing a 16k page takes up to 500 ms
    //
    watchdog_set_timeout(1000);
    FLASH_Unlock();
    FLASH_EraseSector(PARAM_SECTOR0, VoltageRange_3);
    watchdog_set_timeout(WATCHDOG_TIMEOUT);

    int count = 0;
    uint32_t addr = PARAM_BASE0 + 8;

    for (int i=0; i<param_count; i++) {
        const struct param_info *p = &param_table[i];

        if (p->noeeprom || p->readonly)
            continue;

        FLASH_ProgramWord(addr + 0, p->id);

        if (p->type == PTYPE_FLOAT)
            FLASH_ProgramWord(addr + 4, *(uint32_t*)p->flt.ptr);
        else
            FLASH_ProgramWord(addr + 4, *(uint32_t*)p->i32.ptr);

        addr += 8;
        count++;
    }

    FLASH_ProgramWord(PARAM_BASE0 + 4, count * 8 );
    FLASH_ProgramWord(PARAM_BASE0 + 0, PARAM_MAGIC);
    FLASH_Lock();

    printf("%d/%d.\n", count, param_count);
}


void param_clear(void)
{
    printf("Clearing parameters.. ");

    FLASH_Unlock();
    FLASH_ProgramWord(PARAM_BASE0, 0);
    FLASH_Lock();

    printf("ok.\n");
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "util.h"
#include <stdlib.h>


static void param_print(const struct param_info *p)
{
    if (p->type == PTYPE_FLOAT)
        printf("sp %-5d %14f  ", p->id, *p->flt.ptr);
    else
        printf("sp %-5d %14d  ", p->id, *p->i32.ptr);

    printf("# %s", p->name);
    if (p->unit)
        printf(" [%s]", p->unit);

    printf("\n");
}



static void cmd_gp(int argc, char *argv[])
{
    if (argc < 2)
        goto usage;

    for (int i=1; i<argc; i++) {
        int first, last;
        if (!parse_range(argv[i], &first, &last))
            goto usage;

        int found = 0;
        for (int j=0; j<param_count; j++) {
            const struct param_info *p = &param_table[j];
            if (p->id >= first && p->id <= last) {
                param_print(p);
                found++;
            }
        }

        if (!found)
            printf("%s: no matching parameters.\n", argv[i]);
    }
    return;

usage:
    printf("usage: %s <id|[id]:[id]> [id|[id]:[id]] ..\n", argv[0]);
}


static param_error param_parse(const struct param_info *p, const char *s)
{
    char  *endp;
    double value;

    if (p->type == PTYPE_FLOAT)
        value = strtod(s, &endp);
    else {
        // strtol sets errno and returns LONG_MIN/MAX for out-of range numbers.
        //
        errno = 0;
        value = strtol(s, &endp, 0);
        if (errno == ERANGE)
            value = value > 0 ? INFINITY : -INFINITY;
    }

    if (*endp)
        return PERR_INVALID_FORMAT;

    return param_set(p->id, value);
}


static void cmd_sp(int argc, char *argv[])
{
    if (argc < 3 || argc%2 != 1)
        goto usage;

    for (int i=1; i<argc; i += 2) {
        char *endp;
        int  id = strtol(argv[i], &endp, 0);
        if (*endp)
            goto usage;

        param_error  err = PERR_OK;
        const struct param_info *p = param_get_info(id);

        if (p)
            err = param_parse(p, argv[i+1]);
        else
            err = PERR_UNKNOWN_ID;

        if (err != PERR_OK) {
            printf("%d: %s", id, param_strerr(err));
            if (err == PERR_INVALID_VALUE) {
                if (p->type == PTYPE_FLOAT)
                    printf(" [%g .. %g]", p->flt.min, p->flt.max);
                else
                    printf(" [%d .. %d]", p->i32.min, p->i32.max);
            }
            printf("\n");
            break;
        }
    }
    return;

usage:
    printf("usage: %s <id> <value> [<id> <value>] ..\n", argv[0]);
}


SHELL_CMD(gp,           (cmdfunc_t)cmd_gp,              "Get parameter"   )
SHELL_CMD(sp,           (cmdfunc_t)cmd_sp,              "Set parameter"   )
SHELL_CMD(param_reset,  (cmdfunc_t)param_set_defaults,  "Reset to defaults" )
SHELL_CMD(param_load,   (cmdfunc_t)param_load,          "Load parameters" )
SHELL_CMD(param_save,   (cmdfunc_t)param_save,          "Save parameters" )
SHELL_CMD(param_clear,  (cmdfunc_t)param_clear,         "Clear parameters")
