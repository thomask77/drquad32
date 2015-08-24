#pragma once

#include <stdint.h>
#include <math.h>

#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define FIELD_SIZEOF(t, f)  (sizeof(((t*)0)->f))

#define STATIC_ASSERT(x)    _Static_assert(x, #x)

#define STRINGIFY_(x)       #x
#define STRINGIFY(x)        STRINGIFY_(x)


#define clamp(x, min, max)      \
( { typeof (x) _x   = (x);      \
    typeof (x) _min = (min);    \
    typeof (x) _max = (max);    \
    _x < _min ? _min : (_x > _max ? _max : _x); \
} )


#define deadband(x, band)       \
( { typeof (x) _x    = (x);     \
    typeof (x) _band = (band);  \
    _x < - _band                \
    ? _x + _band                \
    : _x > _band                \
      ? _x - _band              \
      : 0;                      \
} )


#define lambda(return_type, function_body) \
( { \
      return_type __fn__ function_body \
          __fn__; \
} )


/**
 * Wrap angle to  -M_PI < x <= M_PI
 *
 */
inline float wrap_pi(float x)
{
    // avoid using floorf() for performance reasons
    //
    if (x >= 0)
        return x - M_TWOPI * (int)((x + M_PI) / M_TWOPI);
    else
        return x - M_TWOPI * (int)((x - M_PI) / M_TWOPI);
}


/**
 * Wrap angle to  0 < x <= M_TWOPI
 *
 */
inline float wrap_twopi(float x)
{
    // avoid using floorf() for performance reasons
    //
    if (x >= 0)
        return x - M_TWOPI * (int)(x / M_TWOPI);
    else
        return x - M_TWOPI * (int)((x - M_TWOPI) / M_TWOPI);
}


/**
 * Statistics functions
 *
 */
struct stats {
    int    n;                ///< Number of samples processed
    double min, max;         ///< Minimum and maximum values
    double sum, sqsum;       ///< Sum and sum-of-squares
    double mean, var, std;   ///< Mean, variance and standard deviation
};


void  stats_update(struct stats *s, double val);
void  stats_reset(struct stats *s);


/**
 * PID controller
 *
 */
struct pid_ctrl {
    float   kp, ki, kd, kaw;
    float   dt, min, max;

    float   u, i, e_1;
};


static inline float pid_update(struct pid_ctrl *pid, float e, float u)
{
    u += pid->kp * e;
    u += pid->i;
    u += pid->kd * (e - pid->e_1) / pid->dt;

    pid->u = clamp(u, pid->min, pid->max);
    pid->i += pid->ki  * e * pid->dt;
    pid->i -= pid->kaw * (u - pid->u);

    pid->e_1 = e;

    return pid->u;
}


static inline void pid_reset(struct pid_ctrl *pid)
{
    pid->u   = 0;
    pid->i   = 0;
    pid->e_1 = 0;
}


int   parse_range(const char *arg, int *first, int *last);
char  *strnbar(char *s, int len, float value, float min, float max);
void  hexdump(const void *addr, unsigned len);
uint32_t hsv_to_rgb(uint32_t hsv);

