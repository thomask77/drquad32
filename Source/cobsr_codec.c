#include "cobsr_codec.h"
#include "cobsr.h"
#include <errno.h>


// TODO: cobs encode/decode direkt integrieren, zero-copy rx/tx machen
//
static char slow_buf[1024];


int cobsr_encode_rb(struct ringbuf *rb, const void *data, size_t len)
{
    if (rb_bytes_free(rb) < COBSR_ENCODE_DST_BUF_LEN_MAX(len) + 1) {
        errno = EWOULDBLOCK;
        return -1;
    }

    ssize_t enc_len = cobsr_encode(slow_buf, sizeof(slow_buf), data, len);

    slow_buf[enc_len++] = 0;
    rb_write(rb, slow_buf, enc_len);

    return len;

    // later:
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
}


int cobsr_decode_rb(struct cobsr_codec *c)
{
    // gucken, ob eine \0 dazu gekommen ist.
    // wenn ja, alles in lineares array kopieren, cobsr_decode aufrufen
    //
    // r�ckgabewert 0, wenn nichts da ist.
    // ansonsten l�nge des empfangenen frames
    //
    return 0;
}


