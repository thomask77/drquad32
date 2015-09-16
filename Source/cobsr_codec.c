#include "cobsr_codec.h"
#include "cobsr.h"
#include "errors.h"
#include <limits.h>
#include <stdbool.h>

// TODO: cobs encode/decode direkt integrieren, zero-copy rx/tx machen
//
// void *ptr1, *ptr2;
// int   len1, len2;
//
// int r = rb_get_pointers(rb, RB_WRITE, len, &ptr1, &len1, &ptr2, &len2);
// if (r != len)
//   return 0;
//
// .. code into ptr1/2 ..
//
// rb_commit(rb, coded length)
//
// immer so viel rein wie gerade passt geht leider nicht..
// (look-ahead beim kodieren notwendig!)
//

static char tx_buf[1024];
static char rx_buf[1024];
static int  rx_pos;


int cobsr_encode_rb(struct ringbuf *rb, const void *data, size_t len)
{
    if (rb_bytes_free(rb) < COBSR_ENCODE_DST_BUF_LEN_MAX(len) + 1) {
        errno = EWOULDBLOCK;
        return -1;
    }

    int n = cobsr_encode(tx_buf, sizeof(tx_buf)-1, data, len);
    if (n >= 0) {
        tx_buf[n++] = 0;
        rb_write(rb, tx_buf, n);
        return len;
    }
    else {
        return -1;
    }
}


int cobsr_decode_rb(struct ringbuf *rb, void *data, size_t len)
{
    char *ptr1;
    size_t len1;

    rb_get_pointers(rb, RB_READ, sizeof(rx_buf), (void*)&ptr1, &len1, NULL, NULL);

    bool eop = false;

    int i;
    for (i=0; i<len1; i++) {
        char c = *ptr1++;
        if (c == 0) {
            eop = true;
            break;
        }

        if (rx_pos < sizeof(rx_buf))
            rx_buf[rx_pos++] = c;
    }

    rb_commit(rb, RB_READ, i);


    int ret = 0;
    if (eop) {
        // end-of-packet received
        //
        if (rx_pos < sizeof(rx_buf)) {
            // decode packet
            //
            ret = cobsr_decode(data, len, rx_buf, rx_pos);
        }
        else {
            // overrun
            //
            errno = EMSG_TOO_LONG;
            ret = -1;
        }
        rx_pos = 0;
    }

    return ret;
}


