#pragma once

#include <unistd.h>
#include "msg_structs.h"

typedef enum {
    MSG_ERR_OK          =  0,
    MSG_ERR_TIMEOUT     = -1,
    MSG_ERR_TOO_LONG    = -2,
    MSG_ERR_COBS        = -3,
    MSG_ERR_TOO_SHORT   = -4,
    MSG_ERR_CRC         = -5
} msg_err;

const char *msg_strerr(msg_err err);
msg_err   msg_recv(struct msg_header *msg);
msg_err   msg_send(struct msg_header *msg);
