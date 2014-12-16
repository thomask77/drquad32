#include "sensors.h"
#include "i2c_mpu9150.h"
#include "i2c_ak8975.h"
#include "i2c_bmp180.h"
#include "ustime.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>

/**
 * Design rationale
 *
 * The individual sensor drivers (i2c_mp9150, etc.) return
 * data in SI units with factory calibration (if available).
 *
 * System calibration is done by sensors.c and should be
 * independent of the actual sensors ICs used.
 *
 */
struct sensor_calib sensor_calib = {
    .acc_gain     = { 1, 1, 1 },
    .acc_offset   = { 0, 0, 0 },
    .gyro_gain    = { 1, 1, 1 },
    .gyro_offset  = { 0, 0, 0 },
    .mag_gain     = { 1, 1, 1 },
    .mag_offset   = { 0, 0, 0 },
    .temp_gain    = 1,
    .temp_offset  = 0
};

// .pressure_qfe: pressure at field evelation
//      The barometric altimeter setting that will cause an altimeter
//      to read zero when at the reference datum of a particular airfield
//
// .pressure_qnh:
//      The barometric altimeter setting that will cause the altimeter
//      to read airfield elevation when on the airfield.
//

static struct  mpu9150_regs   mpu9150_regs;
static struct  ak8975_regs    ak8975_regs;
static struct  bmp180_regs    bmp180_regs;

static struct  mpu9150_data   mpu9150_data;
static struct  ak8975_data    ak8975_data;
static struct  bmp180_data    bmp180_data;

static struct  sensor_data    sensor_data;
static SemaphoreHandle_t      sensor_data_sem;


static void poll_i2c(void)
{
    static int state;

    mpu9150_read(&mpu9150_regs);

    switch (state) {
    case 0:
    case 10:
        ak8975_read(&ak8975_regs);
        ak8975_start_single();
        break;

    case 1:
        bmp180_read(&bmp180_regs);
        bmp180_start_up();
        break;

    case 16:
        bmp180_read(&bmp180_regs);
        bmp180_start_ut();
        break;
    }

    if (++state == 20)
        state = 0;
}


void sensor_read(struct sensor_data *d)
{
    xSemaphoreTake(sensor_data_sem, portMAX_DELAY);

    memcpy(d, &sensor_data, sizeof(*d));

    xSemaphoreGive(sensor_data_sem);
}


void sensor_task(void *param)
{
    sensor_data_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(sensor_data_sem);

    mpu9150_init();
    ak8975_init();
    bmp180_init();

    for (;;) {

        // I/O-Bound sensor polling
        //
        poll_i2c();

        // Convert to SI units and apply calibration
        // TODO: Move to a separate task
        //
        mpu9150_convert(&mpu9150_data, &mpu9150_regs);
        ak8975_convert(&ak8975_data, &ak8975_regs);
        bmp180_convert(&bmp180_data, &bmp180_regs);

        xSemaphoreTake(sensor_data_sem, portMAX_DELAY);

        struct sensor_data *d = &sensor_data;

        d->clipflags = mpu9150_data.clipflags | ak8975_data.clipflags | bmp180_data.clipflags;

        d->acc  = vec3f_fma(mpu9150_data.acc , sensor_calib.acc_gain , sensor_calib.acc_offset );
        d->gyro = vec3f_fma(mpu9150_data.gyro, sensor_calib.gyro_gain, sensor_calib.gyro_offset);
        d->mag  = vec3f_fma(ak8975_data.mag  , sensor_calib.mag_gain , sensor_calib.mag_offset );

        d->gyro_temp = mpu9150_data.temp * sensor_calib.temp_gain + sensor_calib.temp_offset;

        d->baro_temp = bmp180_data.temp;
        d->pressure  = bmp180_data.pressure;

        xSemaphoreGive(sensor_data_sem);

        vTaskDelay(1);
    }
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "ansi.h"
#include "util.h"
#include "syscalls.h"
#include <stdio.h>
#include <string.h>

static void cmd_sensor_show(int argc, char *argv[])
{
    char bar[31];

    enum {
        MODE_GRAPH,
        MODE_CSV,
        MODE_RAW
    } mode = MODE_GRAPH;

    if (argc == 2) {
        if (!strcmp(argv[1], "-g"))
            mode = MODE_GRAPH;
        else if (!strcmp(argv[1], "-c"))
            mode = MODE_CSV;
        else if (!strcmp(argv[1], "-r"))
            mode = MODE_RAW;
        else if (!strcmp(argv[1], "-h"))
            goto usage;
        else
            goto usage;
    }

    if (mode == MODE_GRAPH)
        printf(ANSI_CLEAR ANSI_CURSOR_OFF);

    while (!stdin_chars_avail()) {
        struct sensor_data d;
        sensor_read(&d);

        // order must be consistent with clipflags!
        //
        const float values[] = {
            d.acc.x, d.acc.y, d.acc.z, d.gyro_temp,
            d.gyro.x, d.gyro.y, d.gyro.z,
            d.mag.x * 1e6, d.mag.y * 1e6, d.mag.z * 1e6,
            d.pressure
        };

        switch (mode) {
        case MODE_GRAPH:
            printf(ANSI_HOME);
            printf("acc [m/s^2]\n");
            printf("%8.3f [%s]\n", d.acc.x, strnbar(bar, sizeof(bar), d.acc.x,  -15, 15));
            printf("%8.3f [%s]\n", d.acc.y, strnbar(bar, sizeof(bar), d.acc.y,  -15, 15));
            printf("%8.3f [%s]\n", d.acc.z, strnbar(bar, sizeof(bar), d.acc.z,  -15, 15));
            printf("\n");

            printf("gyro [rad/s]\n");
            printf("%8.3f [%s]\n", d.gyro.x, strnbar(bar, sizeof(bar), d.gyro.x, -M_TWOPI, M_TWOPI));
            printf("%8.3f [%s]\n", d.gyro.y, strnbar(bar, sizeof(bar), d.gyro.y, -M_TWOPI, M_TWOPI));
            printf("%8.3f [%s]\n", d.gyro.z, strnbar(bar, sizeof(bar), d.gyro.z, -M_TWOPI, M_TWOPI));
            printf("%8.3f °C\n",   d.gyro_temp);
            printf("\n");

            printf("mag [uT]\n");
            printf("%8.3f [%s]\n", d.mag.x * 1e6, strnbar(bar, sizeof(bar), d.mag.x * 1e6, -100, 100));
            printf("%8.3f [%s]\n", d.mag.y * 1e6, strnbar(bar, sizeof(bar), d.mag.y * 1e6, -100, 100));
            printf("%8.3f [%s]\n", d.mag.z * 1e6, strnbar(bar, sizeof(bar), d.mag.z * 1e6, -100, 100));
            printf("\n");

            printf("baro [hPa]\n");
            printf("%8.3f [%s]\n", d.pressure, strnbar(bar, sizeof(bar), d.pressure, 950, 1050));
            printf("%8.3f °C\n",   d.baro_temp);
            printf("\n");

            break;

        case MODE_CSV:
             for (int i=0; i<ARRAY_SIZE(values); i++) {
                 if (d.clipflags & (1<<i))
                     printf(ANSI_BG_RED);

                 printf("%8.3f, ", values[i]);

                 if (d.clipflags & (1<<i))
                     printf(ANSI_NORMAL);
             }
             printf("\n");
             break;

        case MODE_RAW:
            for (int i=0; i<ARRAY_SIZE(values); i++) {
                uint32_t i32;
                memcpy(&i32, &values[i], 4);
                printf("%08lx", i32);
            }
            printf("\n");
            break;
        }

        vTaskDelay(100);
    }

    if (mode == MODE_GRAPH)
        printf(ANSI_CURSOR_ON);

    return;

usage:
    printf("usage: %s [-h|-g|-c|-r]\n", argv[0]);
}


SHELL_CMD(sensor_show, (cmdfunc_t)cmd_sensor_show, "Show sensor data")
