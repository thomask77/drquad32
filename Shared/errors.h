#pragma once

#include <errno.h>

// newlib defines __ELASTERROR as 2000
//

#define ERROR_ID_MAP(X) \
    /* Message Errors */ \
    X( 2000, EMSG_TIMEOUT,        "Message timed out" ) \
    X( 2001, EMSG_TOO_LONG,       "Message too long" ) \
    X( 2002, EMSG_TOO_SHORT,      "Message too short" ) \
    X( 2003, EMSG_CRC,            "Message checksum failure" ) \
    X( 2004, EMSG_UNKNOWN,        "Unknown message" ) \
    /* Bootloader errors */ \
    X( 2100, EBOOT_INACTIVE,      "Bootloader not active" ) \
    X( 2101, EBOOT_FLASH,         "Flash error" ) \
    X( 2102, EBOOT_RANGE,         "Invalid address range" ) \
    X( 2103, EBOOT_CHECKSUM,      "Invalid checksum" ) \
    X( 2104, EBOOT_MAGIC,         "Invalid magic" ) \
    /* STM32 Flash controller errors */ \
    X( 2200, EFLASH_BUSY,         "STM32 Flash busy" ) \
    X( 2201, EFLASH_RD,           "STM32 Flash proprietary readout protection error" ) \
    X( 2202, EFLASH_PGS,          "STM32 Flash programming sequence error" ) \
    X( 2203, EFLASH_PGP,          "STM32 Flash programming parallelism error " ) \
    X( 2204, EFLASH_PGA,          "STM32 Flash programming alignment error" ) \
    X( 2205, EFLASH_WRP,          "STM32 Flash write protection error" ) \
    X( 2206, EFLASH_PROGRAM,      "STM32 Flash programming error" ) \
    X( 2207, EFLASH_OPERATION,    "STM32 Flash operation error" ) \
    X( 2208, EFLASH_COMPLETE,     "STM32 Flash complete (no error)" ) \


#define ERROR_ID_ENUM_FN(ID, NAME,  HELP)   \
    NAME = ID,


enum error_id {
    ERROR_ID_MAP(ERROR_ID_ENUM_FN)
};

#ifdef __cplusplus
extern "C" {
#endif

const char *_user_strerror(int errnum);

#ifdef __cplusplus
}
#endif

