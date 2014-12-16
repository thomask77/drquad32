#include <math.h>
#include <stdio.h>


#define TEST(x) \
    printf("wrap_pi(%20s) = %12f %12f %12f\n", \
        #x, wrap_pi_floor(x), wrap_pi(x), wrap_twopi(x) \
    );


// -PI < x <= PI
//
float wrap_pi_floor(float x)
{
    return x - M_TWOPI * floorf( (x + M_PI) / M_TWOPI );
}


// -M_PI < x <= M_PI
//
float wrap_pi(float x)
{
    if (x >=0)
        return x - M_TWOPI * (int)((x + M_PI) / M_TWOPI);
    else
        return x - M_TWOPI * (int)((x - M_PI) / M_TWOPI);
}


// 0 < x <= M_TWOPI
//
float wrap_twopi(float x)
{
    if (x >=0)
        return x - M_TWOPI * (int)(x / M_TWOPI);
    else
        return x - M_TWOPI * (int)((x - M_TWOPI) / M_TWOPI);
}


void main()
{
    TEST(0);
    TEST(1);
    TEST(3.14);
    TEST(3.15);
    TEST(M_PI);
    TEST(2 * M_PI - 0.01);
    TEST(2 * M_PI + 0.01);
    TEST(2 * M_PI);
    TEST(1234.25 * M_TWOPI);
    TEST(1234.75 * M_TWOPI);

    TEST(-0);
    TEST(-1);
    TEST(-3.14);
    TEST(-3.15);
    TEST(-M_PI);
    TEST(-2 * M_PI - 0.01);
    TEST(-2 * M_PI + 0.01);
    TEST(-2 * M_PI);
    TEST(-1234.25 * M_TWOPI);
    TEST(-1234.75 * M_TWOPI);
}
