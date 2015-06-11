#include "Shared/msg_structs.h"
#include "Shared/crc16.h"
#include "Shared/cobsr.h"

#include "command.h"

#include "FreeRTOS.h"
#include "task.h"

#include <math.h>
#include <unistd.h>


// COBSR(CRC + ID + MSG_MAX_DATA_SIZE) + End-of-packet
//
#define MAX_BUF_LENGTH  \
    ( COBSR_ENCODE_DST_BUF_LEN_MAX(2 + 2 + MSG_MAX_DATA_SIZE) + 1 )


static uint8_t tx_buf[MAX_BUF_LENGTH];


/**
 * Calculate CRC header field over ID and data
 *
 */
static crc16_t msg_calc_crc(const struct msg_header *msg)
{
    crc16_t crc = crc16_init();
    crc = crc16_update(crc, (uint8_t*)&msg->id, 2 + msg->data_len);
    crc = crc16_finalize(crc);
    return crc;
}


int  msg_send(struct msg_header *msg)
{
    msg->crc = msg_calc_crc(msg);

    int res = cobsr_encode(
        tx_buf, sizeof(tx_buf) - 1,         // 1 byte for end-of-packet
        &msg->crc, 2 + 2 + msg->data_len    // +CRC +ID
    );

    if (res < 0)
        return -1;

    // add end-of-packet marker
    //
    tx_buf[res++] = 0;

    write(STDOUT_FILENO, tx_buf, res);

    return msg->data_len;
}


void send_imu_data(void)
{
    struct msg_imu_data msg;
    msg.h.id = MSG_ID_IMU_DATA;
    msg.h.data_len = sizeof(struct msg_imu_data) -
                        sizeof(struct msg_header);

    float t = xTaskGetTickCount() * M_TWOPI / 1000.0;

    msg.acc_x       = sinf(t + 0.0  * M_TWOPI) * 1;
    msg.acc_y       = sinf(t + 0.1  * M_TWOPI) * 1;
    msg.acc_z       = sinf(t + 0.2  * M_TWOPI) * 1;
    msg.gyro_x      = sinf(t + 0.3  * M_TWOPI) * 2;
    msg.gyro_y      = sinf(t + 0.4  * M_TWOPI) * 2;
    msg.gyro_z      = sinf(t + 0.5  * M_TWOPI) * 2;
    msg.mag_x       = sinf(t + 0.6  * M_TWOPI) * 3;
    msg.mag_y       = sinf(t + 0.7  * M_TWOPI) * 3;
    msg.mag_z       = sinf(t + 0.8  * M_TWOPI) * 3;
    msg.baro_hpa    = sinf(t + 0.9  * M_TWOPI) * 4;
    msg.baro_temp   = sinf(t + 0.95 * M_TWOPI) * 4;

    msg_send(&msg.h);
}


static void cmd_test(void)
{
    send_imu_data();
}


SHELL_CMD(test, (cmdfunc_t)cmd_test, "test")




