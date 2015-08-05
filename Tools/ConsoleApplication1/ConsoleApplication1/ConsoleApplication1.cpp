// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdint.h>

// raw:   "00 93 01 c0 03 0a 72 65 73 65 74 0a"
// cobs:  "01 0c 93 01 c0 03 0a 72 65 73 65 74 0a 00"


// TODO: 
//   Decode from ringbuf to limited length buffer
//   Encode to ringbuf (size known in advance)
//

void emit(uint8_t c)
{
	printf("%02x ", c);
}

uint8_t last_len, len;


int decode_char(uint8_t c)
{

	if (c == 0) {
        if (len > 1)
            emit(last_len);
        len = 0;
        return 1;
    }

    if (len == 0) {
        last_len = c;
        len = c;
        return 0;
    }

    if (--len) {
        emit(c);
        return 0;
    }
    else {
        emit(0);
		return decode_char(c);
    }
}


int _tmain(int argc, _TCHAR* argv[])
{
	for (int i = 0; i < 14; i++)
		decode_char( "\x01\x0c\x93\x01\xc0\x03\x0a\x72\x65\x73\x65\x74\x0a\x00"[i] );

	for (;;);
	return 0;
}

