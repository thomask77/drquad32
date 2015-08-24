#if 0

#include "led_ws2812.h"
#include "gpn_foo.h"
#include "stm32f4xx_conf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "util.h"
#include "syscalls.h"
#include "bldc_task.h"
#include "sensors.h"
#include <math.h>

union pixel
{
   struct {
       uint8_t  b;
       uint8_t  g;
       uint8_t  r;
       uint8_t  a;
   };
   uint32_t     argb;
};

static union pixel bitmap[72];

int ws2812_brightness;


static struct sensor_data   sensor_data;


void gpn_blink_1(void)
{
    float t = xTaskGetTickCount() * M_TWOPI / 1000;

    for (int i=0; i < ARRAY_SIZE(bitmap); i++) {
        float x = i * M_TWOPI / ARRAY_SIZE(bitmap);

        int   r = sinf(-x + t * 0.65424) * 256;
        int   g = sinf(-x + t * 0.43232) * 256;
        int   b = sinf(-x + t * 0.67652) * 256;

        bitmap[i].r = clamp(r * ws2812_brightness / 256, 0, 255);
        bitmap[i].g = clamp(g * ws2812_brightness / 256, 0, 255);
        bitmap[i].b = clamp(b * ws2812_brightness / 256, 0, 255);
    }
}


void gpn_blink_2(void)
{
    for (int i=0; i < ARRAY_SIZE(bitmap); i++) {
        float r = -sensor_data.acc.x / STANDARD_GRAVITY;
        float g = -sensor_data.acc.y / STANDARD_GRAVITY;
        float b = sensor_data.acc.z / STANDARD_GRAVITY;

        bitmap[i].r = clamp(r * ws2812_brightness, 0, 255);
        bitmap[i].g = clamp(g * ws2812_brightness, 0, 255);
        bitmap[i].b = clamp(b * ws2812_brightness, 0, 255);
    }
}


void gpn_blink_3(void)
{
    for (int i=ARRAY_SIZE(bitmap)-1; i >= 0; i--) {
        bitmap[i].r = bitmap[i-1].r;
        bitmap[i].g = bitmap[i-1].g;
        bitmap[i].b = bitmap[i-1].b;
    }

    float r = -sensor_data.acc.x / STANDARD_GRAVITY;
    float g = -sensor_data.acc.y / STANDARD_GRAVITY;
    float b =  sensor_data.acc.z / STANDARD_GRAVITY;

    bitmap[0].r = clamp(r * ws2812_brightness, 0, 255);
    bitmap[0].g = clamp(g * ws2812_brightness, 0, 255);
    bitmap[0].b = clamp(b * ws2812_brightness, 0, 255);
}


struct ball
{
    union   pixel  color;
    float   p, v;
    float   bounce;
};


struct ball balls[10];


float frand(void)
{
    return rand() / (float)RAND_MAX;
}


void init_balls(void)
{
    for (int i=0; i<ARRAY_SIZE(balls); i++) {
        balls[i].color.r = rand() % 255;
        balls[i].color.g = rand() % 255;
        balls[i].color.b = rand() % 255;
        balls[i].bounce  = frand() * 0.5 + .25;
    }
}


void update_ball(struct ball *b, float a)
{
    const float dt = 1.0 / 50;

    b->p += b->v * dt;
    b->v += a * dt;

    if (b->p > 0.5) {
        b->v = b->v * -b->bounce;
        b->p = 0.5;
    }

    if (b->p < 0) {
        b->v = b->v * -b->bounce;
        b->p = 0;
    }
}


void gpn_blink_4(void)
{
    for (int i=0; i < ARRAY_SIZE(bitmap); i++)  {
        bitmap[i].r = clamp(bitmap[i].r - 32, 0, 255);
        bitmap[i].g = clamp(bitmap[i].g - 32, 0, 255);
        bitmap[i].b = clamp(bitmap[i].b - 32, 0, 255);
    }

    float a = sensor_data.acc.x;

    for (int i=0; i < ARRAY_SIZE(balls); i++) {
        update_ball(&balls[i], a);

        int x = clamp( 144 * balls[i].p, 0, ARRAY_SIZE(bitmap)-1 );

        bitmap[x].r = clamp(bitmap[x].r + balls[i].color.r, 0, 255);
        bitmap[x].g = clamp(bitmap[x].g + balls[i].color.g, 0, 255);
        bitmap[x].b = clamp(bitmap[x].b + balls[i].color.b, 0, 255);
    }

}

#include "../Bitmaps/gpn14_3.c"

union pixel pixel_from_565(uint16_t c)
{
    return (union pixel) {
        .r = ((c >> 11) & 31) << 3,
        .g = ((c >>  5) & 63) << 2,
        .b = ((c >>  0) & 31) << 3
    };
}


void gpn_blink_5(void)
{
    static float phi;
    static float omega_old;
    static int offset;

    float omega = -sensor_data.gyro.z;

    phi += omega * 5e-3;

//    if ((omega_old < 0 && omega > 0) ||
//        (omega_old > 0 && omega < 0)) {
//        offset += 16;
//    }

    if ((omega_old < 0 && omega > 0)) {
        phi = 0;
    }

    // TODO
    // Richtungswechsel erkennen, winkel auf 0 setzen
    // TODO
    // Offset kompensieren

    omega_old = omega;

    for (int i=0; i < ARRAY_SIZE(bitmap); i++)
    {
        int pos = ((int)(320 * phi / M_TWOPI)) + offset;

        pos %= 320;

        bitmap[71-i] = pixel_from_565( gpn14_3[ pos + i * 320 ] );

    }

}


void gpn_blink_batt_low(void)
{
    for (int i=0; i < ARRAY_SIZE(bitmap); i++)
        bitmap[i].argb = 0;

    bitmap[0].r = ((xTaskGetTickCount() / 500) & 1) ? 128 : 0;
}


void gpn_foo_task(void *pvParameters)
{
    TickType_t prev_time = xTaskGetTickCount();

    init_balls();

    for(;;) {
        sensor_read(&sensor_data);

        // gpn_blink_1();
        // gpn_blink_2();
        // gpn_blink_3();
        gpn_blink_4();
        // gpn_blink_5();

        if (bldc_state.u_bat < bldc_params.u_bat_min)
            gpn_blink_batt_low();

        dma_io_clear();
        dma_io_set_ws2812(dma_io_tx_buf, DMA_IO_TX_SIZE, 0x80, (uint32_t*)bitmap, ARRAY_SIZE(bitmap));
        dma_io_send();

        //vTaskDelayUntil(&prev_time, 5);
        vTaskDelayUntil(&prev_time, 20);
    }
}

#endif
