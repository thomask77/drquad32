#ifndef OTP_RECORD_H
#define OTP_RECORD_H

#include <unistd.h>

enum {
    OTP_TYPE_ZERO   = 0,
    OTP_TYPE_MAC    = 1,
    OTP_TYPE_END    = 0xFF
};

int otp_read_rec(int type, void *buf, size_t size);

#endif
