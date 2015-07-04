#include "msg_packet.h"
#include "uart.h"
#include "board.h"

#include "Shared/crc16.h"
#include "Shared/cobsr.h"
#include "Shared/errors.h"

#define PACKET_TIMEOUT  1000     // [ms]

// COBSR(CRC + ID + MSG_MAX_DATA_SIZE) + End-of-packet
//
#define MAX_BUF_LENGTH  \
    ( COBSR_ENCODE_DST_BUF_LEN_MAX(2 + 2 + MSG_MAX_DATA_SIZE) + 1 )


static uint8_t rx_buf[MAX_BUF_LENGTH];
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


int msg_recv(struct msg_header *msg)
{
    uint32_t t0 = tickcount;
    int rx_len = 0;

    for (;;) {
        if (uart_bytes_avail()) {
            char c;
            uart_read(&c, 1);

            if (c == 0) {
                // end-of packet marker
                //
                break;
            }
            else {
                rx_buf[rx_len++] = c;
                if (rx_len == 1) {
                    // Restart packet timeout after first received byte.
                    // This way, we won't interrupt a late arriving packet.
                    //
                    t0 = tickcount;
                }
            }
        }

        if (tickcount - t0 > PACKET_TIMEOUT) {
            errno = EMSG_TIMEOUT;
            return -1;
        }

        if (rx_len >= MAX_BUF_LENGTH) {
            errno = EMSG_TOO_LONG;
            return -1;
        }
    }

    // decode COBS/R
    //
    int res = cobsr_decode(
        &msg->crc, 2 + 2 + MSG_MAX_DATA_SIZE,   // +CRC +ID
        rx_buf, rx_len
    );

    if (res < 0)
        return -1;

    if (res < 2 + 2) {
        errno = EMSG_TOO_SHORT;
        return -1;
    }

    msg->data_len = res -2 -2;     // -CRC -ID

    if (msg_calc_crc(msg) != msg->crc) {
        errno = EMSG_CRC;
        return -1;
    }

    return msg->data_len;
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
    uart_write(tx_buf, res);

    return msg->data_len;
}
