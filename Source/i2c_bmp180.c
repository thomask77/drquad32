#include "i2c_bmp180.h"
#include "i2c_driver.h"
#include "sensors.h"
#include <errno.h>

#define I2C_ADDR            0xEE

// BMP180 register map, taken from BST-BMP180-DS000-09.pdf
//
#define CALIB_PROM          0xAA
#define CHIP_ID             0xD0
#define SOFT_RESET          0xE0
#define CTRL_MEAS           0xF4
#define OUT_MSB             0xF6
#define OUT_LSB             0xF7
#define OUT_XLSB            0xF8

// Oversampling settings
//
#define BMP180_OSS_HIRES

#if   defined(BMP180_OSS_LOWPOWER)
    #define OSS                 0
    #define GET_UT_TIME         5       //  4.5 ms
    #define GET_UP_TIME         5       //  4.5 ms
#elif defined(BMP180_OSS_STANDARD)
    #define OSS                 1
    #define GET_UT_TIME         5       //  4.5 ms
    #define GET_UP_TIME         8       //  7.5 ms
#elif defined(BMP180_OSS_HIRES)
    #define OSS                 2
    #define GET_UT_TIME         5       //  4.5 ms
    #define GET_UP_TIME         14      // 13.5 ms
#elif defined(BMP180_OSS_ULTRAHIRES)
    #define OSS                 3
    #define GET_UT_TIME         5       //  4.5 ms
    #define GET_UP_TIME         26      // 25.5 ms
#else
    #error BMP180: Oversampling not defined
#endif

// Control register bits
//
#define CTRL_MEAS_OSS_MASK  0xC0
#define CTRL_MEAS_OSS_SHIFT 6
#define CTRL_MEAS_SCO       0x20
#define CTRL_MEAS_TEMP      0x0E
#define CTRL_MEAS_PRESSURE  0x14

// Factory calibration values
//
static struct {
    int16_t   ac1, ac2, ac3;
    uint16_t  ac4, ac5, ac6;
    int16_t   b1, b2;
    int16_t   mb, mc, md;
} calib;


static enum {
    READ_NONE,
    READ_TEMP,
    READ_PRESSURE
} bmp180_state;


static int read_calib(void)
{
    uint8_t buf[22];
    int res = i2c_read(I2C_ADDR, CALIB_PROM, buf, sizeof(buf));
    if (res < 0) return res;

    calib.ac1 = (int16_t)((buf[ 0] << 8) | buf[ 1]);
    calib.ac2 = (int16_t)((buf[ 2] << 8) | buf[ 3]);
    calib.ac3 = (int16_t)((buf[ 4] << 8) | buf[ 5]);
    calib.ac4 = (int16_t)((buf[ 6] << 8) | buf[ 7]);
    calib.ac5 = (int16_t)((buf[ 8] << 8) | buf[ 9]);
    calib.ac6 = (int16_t)((buf[10] << 8) | buf[11]);
    calib.b1  = (int16_t)((buf[12] << 8) | buf[13]);
    calib.b2  = (int16_t)((buf[14] << 8) | buf[15]);
    calib.mb  = (int16_t)((buf[16] << 8) | buf[17]);
    calib.mc  = (int16_t)((buf[18] << 8) | buf[19]);
    calib.md  = (int16_t)((buf[20] << 8) | buf[21]);
    return 1;
}


int bmp180_start_ut(void)
{
    bmp180_state = READ_TEMP;

    return i2c_write(I2C_ADDR, CTRL_MEAS, (char[]) {
        CTRL_MEAS_SCO | CTRL_MEAS_TEMP
    }, 1 );
}


int bmp180_start_up(void)
{
    bmp180_state = READ_PRESSURE;

    return i2c_write(I2C_ADDR, CTRL_MEAS, (char[]) {
        (OSS << CTRL_MEAS_OSS_SHIFT) | CTRL_MEAS_SCO | CTRL_MEAS_PRESSURE
    }, 1);
}


int bmp180_read(struct bmp180_regs *regs)
{
    switch (bmp180_state) {

    case READ_TEMP:
        return i2c_read(I2C_ADDR, CTRL_MEAS, &regs->ut, sizeof(regs->ut));

    case READ_PRESSURE:
        return i2c_read(I2C_ADDR, CTRL_MEAS, &regs->up, sizeof(regs->up));

    default:
        return -1;
    }
}


int bmp180_convert(struct bmp180_data *data, const struct bmp180_regs *regs)
{
    if (regs->up.ctrl_meas & CTRL_MEAS_SCO ||
        regs->ut.ctrl_meas & CTRL_MEAS_SCO  )
    {
        return -1;
    }

    int oss = (regs->up.ctrl_meas &  CTRL_MEAS_OSS_MASK) >> CTRL_MEAS_OSS_SHIFT;
    int ut  = (regs->ut.msb << 8) |regs->ut.lsb;
    int up  = ((regs->up.msb << 16) | (regs->up.lsb << 8) | regs->up.xlsb) >> (8 - oss);

    int32_t   t, p;
    int32_t   x1, x2, x3;
    int32_t   b3, b5, b6;
    uint32_t  b4, b7;

    // Calculate true temperature
    //
    x1 = (ut - calib.ac6) * calib.ac5 / 32768;
    x2 = calib.mc * 2048 / (x1 + calib.md);
    b5 = x1 + x2;
    t  = (b5 + 8) / 16;

    // Calculate true pressure
    //
    b6 = b5 - 4000;
    x1 = (calib.b2 * (b6 * b6 / 4096)) / 2048;
    x2 = calib.ac2 * b6 / 2048;
    x3 = x1 + x2;
    b3 = (((calib.ac1 * 4 + x3) << oss) + 2) / 4;
    x1 = calib.ac3 * b6 / 8192;
    x2 = (calib.b1 * (b6 * b6 / 4096)) / 65536;
    x3 = ((x1 + x2) + 2) / 4;
    b4 = (calib.ac4 * (uint32_t)(x3 + 32768)) / 32768;
    b7 = ((uint32_t)up - b3) * (50000 >> oss);
    p  = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
    x1 = (p / 256) * (p / 256);
    x1 = (x1 * 3038) / 65536;
    x2 = (-7357 * p) / 65536;
    p  = p + ((x1 + x2 + 3791) / 16);

    data->clipflags = 0;
    data->temp = t / 10.0;
    data->pressure = p / 100.0;

    return 1;
}


int bmp180_init(void)
{
    printf("Initializing BMP180..\n");

    uint8_t chip_id;

    int res = i2c_read(I2C_ADDR, CHIP_ID, &chip_id, 1);
    if (res < 0) return res;

    printf("  ID: 0x%02x\n", chip_id);

    res = read_calib();
    if (res < 0) return res;

    printf(
        "  AC1, AC2, AC3: %6d, %6d, %6d\n"
        "  AC4, AC5, AC6: %6d, %6d, %6d\n"
        "  B1, B2       : %6d, %6d\n"
        "  MB, MC, MD   : %6d, %6d, %6d\n",
        calib.ac1, calib.ac2, calib.ac3,
        calib.ac4, calib.ac5, calib.ac6,
        calib.b1,  calib.b2,
        calib.mb,  calib.mc,  calib.md
    );

    return res;
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "syscalls.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <stdlib.h>

// TODO: move to sensors.c!

static void bmp180_test(int argc, char *argv[])
{
    // Average pressure above sea level in hPa
    //
    float p0 = STANDARD_PRESSURE;

    if (argc > 2)
        goto usage;

    if (argc > 1)
        p0 = atof(argv[1]);

    while (!stdin_chars_avail()) {
        struct bmp180_regs r;
        struct bmp180_data d;

        bmp180_start_ut();
        vTaskDelay(GET_UT_TIME);
        bmp180_read(&r);

        bmp180_start_up();
        vTaskDelay(GET_UP_TIME);
        bmp180_read(&r);

        bmp180_convert(&d, &r);

        float alt = 44330 * (1 - powf(d.pressure / p0, 1.0/5.255));

        printf("%8.1f°C, %8.2f hPa, %8.3fm\n", d.temp, d.pressure, alt);
        vTaskDelay(100);
    }

    return;

usage:
    printf("usage: %s [hPa_0]\n", argv[0]);
}


SHELL_CMD(bmp180_test, (cmdfunc_t)bmp180_test, "Altimeter test");
SHELL_CMD(bmp180_init, (cmdfunc_t)bmp180_init, "Init BMP180")
