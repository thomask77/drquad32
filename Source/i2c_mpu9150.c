#include "i2c_mpu9150.h"
#include "i2c_driver.h"
#include "sensors.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <math.h>

#define I2C_ADDR            0xD0

// Registers taken from RM-MPU-9150A-00.pdf, v4.2
//
#define SELF_TEST_X         0x0D
#define SELF_TEST_Y         0x0E
#define SELF_TEST_Z         0x0F
#define SELF_TEST_A         0x10

#define SMPLRT_DIV          0x19
#define CONFIG              0x1A
#define GYRO_CONFIG         0x1B
#define ACCEL_CONFIG        0x1C
#define FIFO_EN             0x23
#define INT_PIN_CFG         0x37

#define ACCEL_XOUT_H        0x3B
#define ACCEL_XOUT_L        0x3C
#define ACCEL_YOUT_H        0x3D
#define ACCEL_YOUT_L        0x3E
#define ACCEL_ZOUT_H        0x3F
#define ACCEL_ZOUT_L        0x40
#define TEMP_OUT_H          0x41
#define TEMP_OUT_L          0x42
#define GYRO_XOUT_H         0x43
#define GYRO_XOUT_L         0x44
#define GYRO_YOUT_H         0x45
#define GYRO_ZOUT_H         0x47
#define GYRO_ZOUT_L         0x48

#define PWR_MGMT_1          0x6B
#define WHO_AM_I            0x75

// MPU9150 register bits
//
#define PWR_MGMT_1_DEVICE_RESET     0x80
#define PWR_MGMT_1_SLEEP            0x40
#define PWR_MGMT_1_CYCLE            0x20
#define PWR_MGMT_1_TEMP_DIS         0x08
#define PWR_MGMT_1_CLKSEL_INTERNAL  0x00
#define PWR_MGMT_1_CLKSEL_PLL_X     0x01
#define PWR_MGMT_1_CLKSEL_PLL_Y     0x02
#define PWR_MGMT_1_CLKSEL_PLL_Z     0x03
#define PWR_MGMT_1_CLKSEL_PLL_32    0x04
#define PWR_MGMT_1_CLKSEL_PLL_19M2  0x05
#define PWR_MGMT_1_CLKSEL_STOP      0x07

#define GYRO_CONFIG_XG_ST           0x80
#define GYRO_CONFIG_YG_ST           0x40
#define GYRO_CONFIG_ZG_ST           0x20
#define GYRO_CONFIG_FS_SEL_250      0x00
#define GYRO_CONFIG_FS_SEL_500      0x08
#define GYRO_CONFIG_FS_SEL_1000     0x10
#define GYRO_CONFIG_FS_SEL_2000     0x18

#define ACCEL_CONFIG_XA_ST          0x80
#define ACCEL_CONFIG_YA_ST          0x40
#define ACCEL_CONFIG_ZA_ST          0x20
#define ACCEL_CONFIG_AFS_SEL_2G     0x00
#define ACCEL_CONFIG_AFS_SEL_4G     0x08
#define ACCEL_CONFIG_AFS_SEL_8G     0x10
#define ACCEL_CONFIG_AFS_SEL_16G    0x18

#define CONFIG_DLPF_CFG_256         0x00
#define CONFIG_DLPF_CFG_188         0x01
#define CONFIG_DLPF_CFG_98          0x02
#define CONFIG_DLPF_CFG_42          0x03
#define CONFIG_DLPF_CFG_20          0x04
#define CONFIG_DLPF_CFG_10          0x05
#define CONFIG_DLPF_CFG_5           0x06

#define INT_PIN_CFG_INT_LEVEL       0x80
#define INT_PIN_CFG_INT_OPEN        0x40
#define INT_PIN_CFG_LATCH_INT_EN    0x20
#define INT_PIN_CFG_INT_RD_CLEAR    0x10
#define INT_PIN_CFG_FSYNC_INT_LEVEL 0x08
#define INT_PIN_CFG_FSYNC_INT_EN    0x04
#define INT_PIN_CFG_I2C_BYPASS_EN   0x02

// Default values from the data sheet
//
#define ACC_GAIN_2      (STANDARD_GRAVITY / 16384)
#define ACC_GAIN_4      (STANDARD_GRAVITY /  8192)
#define ACC_GAIN_8      (STANDARD_GRAVITY /  4096)
#define ACC_GAIN_16     (STANDARD_GRAVITY /  2048)

#define TEMP_GAIN       (1.0 / 340.0)
#define TEMP_OFFSET     36.53

#define GYRO_GAIN_250   (M_TWOPI / (360 * 131.0))
#define GYRO_GAIN_500   (M_TWOPI / (360 *  65.5))
#define GYRO_GAIN_1000  (M_TWOPI / (360 *  32.8))
#define GYRO_GAIN_2000  (M_TWOPI / (360 *  16.4))

// Full scale configuration
//
#define GYRO_CONFIG_FS_SEL      GYRO_CONFIG_FS_SEL_2000
#define ACCEL_CONFIG_AFS_SEL    ACCEL_CONFIG_AFS_SEL_8G

#define ACC_GAIN                ACC_GAIN_8
#define GYRO_GAIN               GYRO_GAIN_2000


int mpu9150_read(struct mpu9150_regs *regs)
{
    return i2c_read(I2C_ADDR, ACCEL_XOUT_H, regs, sizeof(*regs));
}


int mpu9150_convert(struct mpu9150_data *data, const struct mpu9150_regs *regs)
{
    // Rotate sensor outputs to match North, East, Down coordinates
    //
    int16_t ay =  ( (regs->acc_xout_h  << 8) | regs->acc_xout_l  );
    int16_t ax =  ( (regs->acc_yout_h  << 8) | regs->acc_yout_l  );
    int16_t az = -( (regs->acc_zout_h  << 8) | regs->acc_zout_l  );

    int16_t t  =  ( (regs->temp_out_h  << 8) | regs->temp_out_l  );

    int16_t gy =  ( (regs->gyro_xout_h << 8) | regs->gyro_xout_l );
    int16_t gx =  ( (regs->gyro_yout_h << 8) | regs->gyro_yout_l );
    int16_t gz = -( (regs->gyro_zout_h << 8) | regs->gyro_zout_l );

    // Check for clipped values
    //
    data->clipflags = 0;
    if (ax == -32768 || ax == 32767) data->clipflags |= CLIP_ACC_X;
    if (ay == -32768 || ay == 32767) data->clipflags |= CLIP_ACC_Y;
    if (az == -32768 || az == 32767) data->clipflags |= CLIP_ACC_Z;
    if (t  == -32768 || t  == 32767) data->clipflags |= CLIP_GYRO_TEMP;
    if (gx == -32768 || gx == 32767) data->clipflags |= CLIP_GYRO_X;
    if (gy == -32768 || gy == 32767) data->clipflags |= CLIP_GYRO_Y;
    if (gz == -32768 || gz == 32767) data->clipflags |= CLIP_GYRO_Z;

    // Convert raw values to SI units
    //
    data->acc.x  = ax * ACC_GAIN;
    data->acc.y  = ay * ACC_GAIN;
    data->acc.z  = az * ACC_GAIN;
    data->temp   = t  * TEMP_GAIN + TEMP_OFFSET;
    data->gyro.x = gx * GYRO_GAIN;
    data->gyro.y = gy * GYRO_GAIN;
    data->gyro.z = gz * GYRO_GAIN;

    return 1;
}


int mpu9150_init(void)
{
    // TODO: Error handling
    //
    printf("Initializing MPU9150..\n");

    uint8_t who_am_i;
    i2c_read(I2C_ADDR, WHO_AM_I, &who_am_i, 1);
    printf("  WHO_AM_I: 0x%02x\n", who_am_i);

    // Reset device
    //
    i2c_write(I2C_ADDR, PWR_MGMT_1,   (char[]){ PWR_MGMT_1_DEVICE_RESET }, 1);
    vTaskDelay(20);
    i2c_write(I2C_ADDR, PWR_MGMT_1,   (char[]){ PWR_MGMT_1_CLKSEL_PLL_X }, 1);
    vTaskDelay(20);

    // Configure gyro and accelerometer
    //
    i2c_write(I2C_ADDR, GYRO_CONFIG,  (char[]){ GYRO_CONFIG_FS_SEL }, 1);
    i2c_write(I2C_ADDR, ACCEL_CONFIG, (char[]){ ACCEL_CONFIG_AFS_SEL }, 1);

    i2c_write(I2C_ADDR, CONFIG,       (char[]){ CONFIG_DLPF_CFG_256 }, 1);
    i2c_write(I2C_ADDR, SMPLRT_DIV,   (char[]){ 0 }, 1);

    // Enable I2C bypass for the magnetometer
    //
    i2c_write(I2C_ADDR, INT_PIN_CFG,  (char[]){ INT_PIN_CFG_I2C_BYPASS_EN }, 1);

    return 1;
}


// -------------------- Shell commands --------------------
//
#include "command.h"

SHELL_CMD(mpu9150_init, (cmdfunc_t)mpu9150_init, "Init MPU9150")
