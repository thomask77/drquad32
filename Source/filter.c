#include "filter.h"
#include <math.h>

// ---------- First-order low pass filter ----------
//

void lp1_set_fc(struct lp1_filter *f, float fc)
{
    const float L = 1 / tanf(M_PI * fc);

    f->b[0] = 1 / (1 + L);
    f->b[1] = f->b[0];
    f->a[0] = 1;    // not used
    f->a[1] = (1 - L) / (1 + L);
}


float lp1_filter(struct lp1_filter *f, float x)
{
    // Shift the old samples
    //
    f->x[1] = f->x[0];
    f->y[1] = f->y[0];

    f->x[0]  = x;

    // Calculate the new output
    //
    f->y[0]  = f->b[0] * f->x[0];
    f->y[0] += f->b[1] * f->x[1] - f->a[1] * f->y[1];

    return f->y[0];
}


void lp1_reset(struct lp1_filter *f, float x)
{
    f->x[0] = f->x[1] = x;
    f->y[0] = f->y[1] = x;
}


// ---------- Second-order low pass filter ----------
//

static void lp2_set_fc_1st(struct lp2_filter *f, float L)
{
    f->b[0] = 1  / (1 + L);
    f->b[1] = f->b[0];
    f->b[2] = 0;
    f->a[0] = 1;    // not used
    f->a[1] = (1 - L) / (1 + L);
    f->a[2] = 0;
}


static void lp2_set_fc_2nd(struct lp2_filter *f, float L, float c2, float c1)
{
    f->b[0] = 1 / (1 + c1*L + c2*L*L);
    f->b[1] = 2 * f->b[0];
    f->b[2] = f->b[0];
    f->a[0] = 1;    // not used
    f->a[1] = 2 * (1 - c2*L*L) / (1 + c1*L + c2*L*L);
    f->a[2] = (1 - c1*L + c2*L*L) / (1 + c1*L + c2*L*L);
}


/**
 * Set cut-off frequency for 2nd-order filters.
 *
 * \note: For 2nd-order filters the gain at fc is 0.5
 *        (instead of sqrt(2))
 *
 */
void lp2_set_fc(struct lp2_filter *f, enum filter_type type, float fc)
{
    const float L = 1 / tanf(M_PI * fc);

    switch (type) {
    case FILTER_1STORDER:       lp2_set_fc_1st(f, L);                   break;
    case FILTER_CRITICALDAMPED: lp2_set_fc_2nd(f, L, 1,      2       ); break;
    case FILTER_BUTTERWORTH:    lp2_set_fc_2nd(f, L, 1,      M_SQRT2 ); break;
    case FILTER_BESSEL:         lp2_set_fc_2nd(f, L, 1.0/3,  1       ); break;
    case FILTER_CHEBYSHEV_0_5:  lp2_set_fc_2nd(f, L, 0.6595, 0.9403  ); break;

    default:
    case FILTER_NONE:
        f->b[0] = f->a[0] = 1;
        f->b[1] = f->a[1] = 0;
        f->b[2] = f->a[2] = 0;
        break;
    }
}


float lp2_filter(struct lp2_filter *f, float x)
{
    // Shift the old samples
    //
    f->x[2] = f->x[1];
    f->y[2] = f->y[1];

    f->x[1] = f->x[0];
    f->y[1] = f->y[0];

    f->x[0] = x;

    // Calculate the new output
    //
    f->y[0]  = f->b[0] * f->x[0];
    f->y[0] += f->b[1] * f->x[1] - f->a[1] * f->y[1];
    f->y[0] += f->b[2] * f->x[2] - f->a[2] * f->y[2];

    return f->y[0];
}


void lp2_reset(struct lp2_filter *f, float x)
{
    f->x[0] = f->x[1] = f->x[2] = x;
    f->y[0] = f->y[1] = f->y[2] = x;
}


// ---------- Moving average filter ----------
//

int32_t avg_filter(struct avg_filter *f, int32_t x)
{
    f->acc -= f->buf[f->index];
    f->buf[f->index] = x;
    f->acc += f->buf[f->index];

    if (++f->index >= f->size)
        f->index = 0;

    return f->y = f->acc / f->size;
}


void avg_reset(struct avg_filter *f, int32_t x)
{
    f->index = 0;
    f->acc   = x * f->size;
    for (int i=0; i<f->size; i++)
        f->buf[i] = x;
}

