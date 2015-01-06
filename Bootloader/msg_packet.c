#include "msg_packet.h"
#include "crc16_sm.h"
#include "uart.h"
#include "board.h"
#include "cobsr.h"

#define PACKET_TIMEOUT  1000     // [ms]

// Message ID + 255 bytes data
//
#define MAX_MESSAGE_LENGTH  (2 + 255)

// COBSR(CRC + MAX_MESSAGE_LENGTH) + End-of-packet
//
#define MAX_BUF_LENGTH  \
    (COBSR_ENCODE_DST_BUF_LEN_MAX(2 + MAX_MESSAGE_LENGTH) + 1)


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


msg_err  msg_recv(struct msg_header *msg)
{
    uint32_t t0 = tickcount;

    int rx_len = 0;
    for (;;) {
        int c = uart_getc();

        if (c == 0) {
            // end-of packet marker
            //
            break;
        }

        if (c >= 0) {
            rx_buf[rx_len++] = c;

            if (rx_len == 1) {
                // Restart packet timeout after first received byte.
                // This way, we won't interrupt a late arriving packet.
                //
                t0 = tickcount;
            }
        }

        if (tickcount - t0 > PACKET_TIMEOUT) {
            // packet timed out
            //
            return MSG_ERR_TIMEOUT;
        }

        if (rx_len >= MAX_BUF_LENGTH)
            return MSG_ERR_TOO_LONG;
    }

    // decode COBS/R
    //
    uint8_t *dst_buf = (uint8_t*)&msg->crc;
    int      dst_len = 2 + 2 + msg->data_len;   // +CRC +ID

    cobsr_decode_result  cobsr_res = cobsr_decode(
        dst_buf, dst_len, rx_buf, rx_len
    );

    if (cobsr_res.status != COBSR_DECODE_OK)
        return MSG_ERR_COBS;

    if (cobsr_res.out_len < 2 + 2)
        return MSG_ERR_TOO_SHORT;

    msg->data_len = cobsr_res.out_len -2 -2;     // -CRC -ID

    // Check CRC
    //
    if (msg_calc_crc(msg) != msg->crc)
        return MSG_ERR_CRC;

    return MSG_ERR_OK;
}


msg_err  msg_send(struct msg_header *msg)
{
    msg->crc = msg_calc_crc(msg);

    cobsr_encode_result  cobsr_res = cobsr_encode(
        tx_buf, sizeof(tx_buf) - 1,                 // 1 byte for end-of-packet
        (uint8_t*)&msg->crc, 2 + 2 + msg->data_len  // +CRC +ID
    );

    if (cobsr_res.status != COBSR_ENCODE_OK)
        return MSG_ERR_COBS;

    // add end-of-packet marker
    //
    tx_buf[cobsr_res.out_len++] = 0;

    uart_write(tx_buf, cobsr_res.out_len);

    return MSG_ERR_OK;
}


const char *msg_strerr(msg_err err)
{
    if (err >= 0)
        return "MSG_ERR_OK";

    switch(err) {
    case MSG_ERR_TIMEOUT:   return "MSG_ERR_TIMEOUT";
    case MSG_ERR_TOO_LONG:  return "MSG_ERR_TOO_LONG";
    case MSG_ERR_COBS:      return "MSG_ERR_COBS";
    case MSG_ERR_TOO_SHORT: return "MSG_ERR_TOO_SHORT";
    case MSG_ERR_CRC:       return "MSG_ERR_CRC";
    default:                return "MSG_ERR_UNKNOWN";
    }
}

