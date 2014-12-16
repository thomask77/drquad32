/**
 * Experimental digital filter test bench.
 * Copyright (c)2003-2013 Thomas Kindler <mail@t-kindler.de>
 *
 * 2013-09-15: tk, re-implemented in plain c for gnuplot-output
 * 2008-09-22: tk, initial implementation in c# for the SiD4AVR project
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

#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

#include "filter.h"

#define FS  20000.0


typedef float (*filter_fun)(void* state, float u);


float rpm_filter(void *state, float u)
{
    // 2nd order critical damped
    //
    const float a = 0.005;

    static float rpm_2, rpm;

    rpm_2 = (1-a) * rpm_2 + a * u;
    rpm   = (1-a) * rpm   + a * rpm_2;

    return rpm;
}


double unwrap(double *last_phase, double phase)
{
    while (phase - *last_phase >  M_PI) phase -= M_TWOPI;
    while (phase - *last_phase < -M_PI) phase += M_TWOPI;
    return *last_phase = phase;
}


/**
 * Measure filter response at a given frequency f
 *
 */
complex double measure(filter_fun filter, void *state, double f)
{
    // 10 complete cycles
    //
    const int N = 10 * FS * M_TWOPI / f;

    complex double output = 0;

    for (int i=0; i<N; i++) {
        static double lo_phase;
        complex double lo = sin(lo_phase) + I*cos(lo_phase);

        output += filter(state, sin(lo_phase)) * lo;

        lo_phase += f * M_TWOPI / FS;
        if (lo_phase >= M_TWOPI)
            lo_phase -= M_TWOPI;
    }

    return 2 * output / N; // WHY * 2 ????
}


void log_sweep(
    filter_fun filter, void *state,
    double f_min, double f_max, int steps, double exp
)
{
    printf(
        "%20s%20s%20s%20s%20s\n",
        "f", "re", "im", "abs", "phase"
    );

    double phase = 0;

    for (int i=0; i<steps; i++) {
        double f = f_min + pow((double)i/(steps-1), exp) * (f_max - f_min);
        complex double z = measure(filter, state, f);

        phase = unwrap(&phase, carg(z));

        printf("%20g%20g%20g%20g%20g\n", f,
            creal(z), cimag(z), cabs(z),  phase * 360 / M_TWOPI
        );
    }

    // data-set separator
    //
    printf("\n\n");
}


int main(int argc, char *argv[])
{

    log_sweep(rpm_filter, NULL, 1, FS/2, 1000, log(10));


    struct lp2_filter f2 = { 0 };
    lp2_set_fc(&f2, FILTER_CRITICALDAMPED, 16.666 / FS);

    log_sweep( lp2_filter, &f2, 1, FS/2, 1000, log(10) );


/*
    struct lp1_filter f1 = { 0 };
    struct lp2_filter f2 = { 0 };

    lp1_set_fc(&f1, 100.0 / FS);
    log_sweep( lp1_filter, &f1, 1, FS/2, 1000, log(10) );

    lp2_set_fc(&f2, FILTER_CRITICALDAMPED, 100.0 / FS);
    log_sweep( lp2_filter, &f2, 1, FS/2, 1000, log(10) );

    lp2_set_fc(&f2, FILTER_CHEBYSHEV_0_5, 100.0 / FS);
    log_sweep( lp2_filter, &f2, 1, FS/2, 1000, log(10) );
*/

    return EXIT_SUCCESS;
}
