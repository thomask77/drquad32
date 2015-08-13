#include "Shared/msg_structs.h"
#include "Shared/crc16.h"
#include "Shared/cobsr.h"

#include "command.h"

#include "FreeRTOS.h"
#include "task.h"

#include <math.h>
#include <unistd.h>


#if 0

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

#endif


