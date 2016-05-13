#pragma once

#include <math.h>
#include <limits.h>
#include <stdint.h>


#define P_FLOAT(_ptr, def_min_max...)  \
    .type   = PTYPE_FLOAT,  \
    .noinit = sizeof(#def_min_max) == 1, \
    .flt    = {             \
        .def = 0,           \
        .min = -INFINITY,   \
        .max = INFINITY,    \
        .ptr = _ptr,        \
        ##def_min_max       \
    },                      \
    .name = #_ptr


#define P_INT32(_ptr, def_min_max...)  \
    .type   = PTYPE_INT32,  \
    .noinit = sizeof(#def_min_max) == 1, \
    .i32    = {             \
        .def = 0,           \
        .min = INT_MIN,     \
        .max = INT_MAX,     \
        .ptr = _ptr,        \
        ##def_min_max       \
    },                      \
    .name = #_ptr


#define P_TEST(_ptr)                \
    .type = _Generic( _ptr,         \
        int*    : PTYPE_INT32,      \
        float*  : PTYPE_FLOAT       \
    )


#define NOINIT      .noinit=1
#define READONLY    .readonly=1
#define NOEEPROM    .noeeprom=1

typedef enum {
    PERR_OK,
    PERR_UNKNOWN_ID,
    PERR_READONLY,
    PERR_INVALID_VALUE,
    PERR_INVALID_FORMAT
} param_error;


typedef enum {
    PTYPE_INT32,
    PTYPE_FLOAT
} param_type;


struct param_info {
    int         id;

    union {
        struct {
            param_type  type     : 4;
            unsigned    noinit   : 1;
            unsigned    readonly : 1;
            unsigned    noeeprom : 1;
        };
        uint32_t flags;
    };

    union {
        struct { int   *ptr, def, min, max; } i32;
        struct { float *ptr, def, min, max; } flt;
    };

    const char *name;
    const char *unit;
    const char *help;
};


const struct param_info *param_get_info(int id);

param_error param_get(int id, double *value);
param_error param_set(int id, double  value);

void param_set_defaults(void);

const char *param_strerr(param_error error);

void param_load(void);
void param_save(void);
void param_clear(void);

