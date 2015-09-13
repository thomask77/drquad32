#include "cobsr_codec.h"
#include "cobsr.h"
#include <errno.h>


int cobsr_encode_x(struct ringbuf *rb, const void *data, size_t len)
{
    static char encbuf[1024];   // TODO: don't

    if (rb_bytes_free(rb) < COBSR_ENCODE_DST_BUF_LEN_MAX(len)) {
        errno = EWOULDBLOCK;
        return -1;
    }

    ssize_t enclen = cobsr_encode(encbuf, sizeof(encbuf), data, len);

    rb_write(rb, encbuf, enclen);

    return len;

    // später:
    //
    // void *ptr1, *ptr2;
    // int   len1, len2;
    //
    // int r = rb_get_pointers(rb, RB_WRITE, len, &ptr1, &len1, &ptr2, &len2);
    // if (r != len)
    //   return 0;
    //
    // .. in ptr1/2 kodieren ..
    //
    // rb_commit(rb, tatsächlich kodierte länge)
    //
    // immer so viel rein wie gerade passt geht leider nicht..
    // (look-ahead beim kodieren notwendig!)
}


int cobsr_decode_x(struct cobsr_codec *c)
{
    // gucken, ob eine \0 dazu gekommen ist.
    // wenn ja, alles in lineares array kopieren, cobsr_decode aufrufen
    //
    // r�ckgabewert 0, wenn nichts da ist.
    // ansonsten l�nge des empfangenen frames
    //
}


// TODO: cobs encode/decode direkt integrieren, zero-copy rx/tx machen
//
