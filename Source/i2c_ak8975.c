#include "i2c_ak8975.h"
#include "i2c_driver.h"
#include "sensors.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#define I2C_ADDR        0x18

// Register map, taken from AK8975.pdf 2010/05
//
#define WIA             0x00
#define INFO            0x01
#define ST1             0x02
#define HXL             0x03
#define HXH             0x04
#define HYL             0x05
#define HYH             0x06
#define HZL             0x07
#define HZH             0x08
#define ST2             0x09
#define CNTL            0x0A
#define RSV             0x0B
#define ASTC            0x0C
#define TS1             0x0D
#define TS2             0x0E
#define I2CDIS          0x0F
#define ASAX            0x10
#define ASAY            0x11
#define ASAZ            0x12

// Register bits
//
#define ST1_DRDY        0x01

#define ST2_DERR        0x04
#define ST2_HOFL        0x08

#define CNTL_MODE_POWER_DOWN    1
#define CNTL_MODE_SINGLE        2
#define CNTL_MODE_SELF_TEST     8
#define CNTL_MODE_FUSE_ROM      15

#define ASTC_SELF       0x40

// Default values from the data sheet
//
#define MAG_GAIN        (1229e-6 / 4096)    // [T/LSB]
#define MEAS_TIME       10                  // [ms]


// Factory calibration values
//
static struct {
    float gain_x;
    float gain_y;
    float gain_z;
} calib;


static int read_calib(void)
{
    int res = 0;

    res = i2c_write(I2C_ADDR, CNTL, (char[]){ CNTL_MODE_FUSE_ROM }, 1);
    if (res < 0) return res;

    uint8_t buf[3];
    res = i2c_read(I2C_ADDR, ASAX, &buf, sizeof(buf));
    if (res < 0) return res;

    calib.gain_x = ((buf[0] - 128) * 0.5) / 128 + 1;
    calib.gain_y = ((buf[1] - 128) * 0.5) / 128 + 1;
    calib.gain_z = ((buf[2] - 128) * 0.5) / 128 + 1;

    res = i2c_write(I2C_ADDR, CNTL, (char[]){ CNTL_MODE_POWER_DOWN }, 1);
    if (res < 0) return res;

    return res;
}


int ak8975_start_single(void)
{
    return i2c_write(I2C_ADDR, CNTL, (char[]){ CNTL_MODE_SINGLE }, 1);
}


int ak8975_read(struct ak8975_regs *regs)
{
    return i2c_read(I2C_ADDR, ST1, regs, sizeof(*regs));
}


int ak8975_convert(struct ak8975_data *data, const struct ak8975_regs *regs)
{
    if ((!regs->st1 & ST1_DRDY) || (regs->st2 & ST2_DERR))
        return -1;

    int16_t x = (regs->hxh << 8) | regs->hxl;
    int16_t y = (regs->hyh << 8) | regs->hyl;
    int16_t z = (regs->hzh << 8) | regs->hzl;

    // Check for clipped values
    //
    data->clipflags = 0;

    if (regs->st2 & ST2_HOFL)
        data->clipflags |= CLIP_MAG_X | CLIP_MAG_Y | CLIP_MAG_Z;

    if (x <= -4096 || x >= 4095) data->clipflags |= CLIP_MAG_X;
    if (y <= -4096 || y >= 4095) data->clipflags |= CLIP_MAG_Y;
    if (z <= -4096 || z >= 4095) data->clipflags |= CLIP_MAG_Z;

    // Convert raw values to SI units
    //
    data->mag.x = x * calib.gain_x * MAG_GAIN;
    data->mag.y = y * calib.gain_y * MAG_GAIN;
    data->mag.z = z * calib.gain_z * MAG_GAIN;

    return 1;
}


int ak8975_init(void)
{
    printf("Initializing AK8975..\n");

    struct { uint8_t wia, info; } regs;

    int res = i2c_read(I2C_ADDR, WIA,  &regs, sizeof(regs));
    if (res < 0) return res;

    printf("  WIA : 0x%02x\n", regs.wia );
    printf("  INFO: 0x%02x\n", regs.info);

    res = read_calib();
    if (res < 0) return res;

    printf("  ASAX: %.3f\n", calib.gain_x);
    printf("  ASAY: %.3f\n", calib.gain_y);
    printf("  ASAZ: %.3f\n", calib.gain_z);

    return 1;
}


// -------------------- Shell commands --------------------
//
#include "command.h"

SHELL_CMD(ak8975_init, (cmdfunc_t)ak8975_init,  "Init AK8975")
