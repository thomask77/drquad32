#pragma once

#include <stdint.h>


enum filter_type {
    FILTER_NONE,
    FILTER_1STORDER,
    FILTER_CRITICALDAMPED,
    FILTER_BUTTERWORTH,
    FILTER_BESSEL,
    FILTER_CHEBYSHEV_0_5
};


/**
 * First-order low pass filter
 *
 */
struct lp1_filter {
    float b[2], a[2];
    float x[2], y[2];
};


/**
 * Second-order low pass filter
 *
 */
struct lp2_filter {
    float b[3], a[3];
    float x[3], y[3];
};


/**
 * Fast moving average filter.
 * Uses integer arithmetic to avoid rounding errors.
 *
 */
struct avg_filter {
    int32_t     *buf;
    uint32_t    size;
    uint32_t    index;
    int32_t     acc;
    int32_t     y;
};


void    lp1_set_fc(struct lp1_filter *f, float fc);
float   lp1_filter(struct lp1_filter *f, float x);
void    lp1_reset(struct lp1_filter *f, float x);

void    lp2_set_fc(struct lp2_filter *f, enum filter_type type, float fc);
float   lp2_filter(struct lp2_filter *f, float x);
void    lp2_reset(struct lp2_filter *f, float x);

int32_t avg_filter(struct avg_filter *f, int32_t x);
void    avg_reset(struct avg_filter *f, int32_t x);

