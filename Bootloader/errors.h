#ifndef ERRORS_H
#define ERRORS_H

#ifndef MAKE_ERROR_TABLE

#include <errno.h>

enum {
    EUSER_BASE = __ELASTERROR,
    #define ENUM(name, desc) name,

#endif


ENUM( EMSG_TIMEOUT,     "Message timed out" )
ENUM( EMSG_COBSR,       "Message encoding failure" )
ENUM( EMSG_TOO_LONG,    "Message too long" )
ENUM( EMSG_TOO_SHORT,   "Message too short" )
ENUM( EMSG_CRC,         "Message checksum failure" )
ENUM( EMSG_UNKNOWN,     "Unknown message" )


#ifndef MAKE_ERROR_TABLE
};
#endif

#undef ENUM

#endif
