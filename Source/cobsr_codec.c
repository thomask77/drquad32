#include "cobsr_codec.h"
#include "cobsr.h"


int cobsr_encode_x(struct cobsr_codec *c)
{
    // gucken, ob in ringbuf genug platz ist (worst-case-betrachtung)
    // wenn ja: daten mit cobsr kodieren, und in ringbuf kopieren
    //
    // rückgabewert 0 wenn's nicht passt
    // sonst länge des versendeten frames (unkodiert)
    //
}


int cobsr_decode_x(struct cobsr_codec *c)
{
    // gucken, ob eine \0 dazu gekommen ist.
    // wenn ja, alles in lineares array kopieren, cobsr_decode aufrufen
    //
    // rückgabewert 0, wenn nichts da ist.
    // ansonsten länge des empfangenen frames
    //
}


// TODO: cobs encode/decode direkt integrieren, zero-copy rx/tx machen
//
