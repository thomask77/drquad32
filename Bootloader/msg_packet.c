#include "msg_packet.h"
#include "uart.h"
#include "board.h"
#include "crc16.h"
#include "cobsr_codec.h"
#include "errors.h"

#define PACKET_TIMEOUT  1000     // [ms]


// COBSR(CRC + ID + MSG_MAX_DATA_SIZE)
//
#define MAX_BUF_LENGTH  \
    ( COBSR_ENCODE_DST_BUF_LEN_MAX(2 + 2 + MSG_MAX_DATA_SIZE) )


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

    struct cobsr_decoder_state dec = {
        .out = (char*)&msg->crc,
        .out_end = (char*)&msg->crc + 2 + 2 + MSG_MAX_DATA_SIZE
    };

    for (;;) {
        if (uart_bytes_avail()) {
            char c;
            uart_read(&c, 1);

            dec.in = &c;
            dec.in_end = &c + 1;
            if (cobsr_decode(&dec))
                break;
        }

        if (tickcount - t0 > PACKET_TIMEOUT) {
            errno = EMSG_TIMEOUT;
            return -1;
        }
    }

    size_t out_len = dec.out - (char*)&msg->crc;

    if (out_len < 2 + 2) {
        errno = EMSG_TOO_SHORT;
        return -1;
    }

    msg->data_len = out_len -2 -2;     // -CRC -ID

    if (msg_calc_crc(msg) != msg->crc) {
        errno = EMSG_CRC;
        return -1;
    }

    return msg->data_len;
}


int  msg_send(struct msg_header *msg)
{
    msg->crc = msg_calc_crc(msg);

    struct cobsr_encoder_state enc = {
        .in = (char*)&msg->crc,
        .in_end = (char*)&msg->crc + 2 + 2 + msg->data_len,
        .out = (char*)tx_buf,
        .out_end = (char*)tx_buf + sizeof(tx_buf)
    };

    cobsr_encode(&enc);

    uart_write(tx_buf, enc.out - (char*)tx_buf);

    return msg->data_len;
}

