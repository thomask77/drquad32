#if     !defined(ERRORS_H) || defined(_ERR_MAKE_STRING_TABLE)
#define ERRORS_H

/**
 * ATTENTION:
 *   This file is used by the DrQuad application as well as the
 *   Bootloader and QuadControl. Check all three if you make
 *   modifications.
 */

#include <errno.h>

#ifndef _ERR_MAKE_STRING_TABLE

#ifndef __ELASTERROR
// newlib has this defined, glibc has not
//
#define __ELASTERROR    2000
#endif

#define _ERR_GROUP(id, value)   id = value,
#define _ERR_NAME(id, desc)     id,

enum error_id {

#endif

_ERR_GROUP( EUSER_BASE, __ELASTERROR )
_ERR_NAME( EMSG_TIMEOUT,        "Message timed out" )
_ERR_NAME( EMSG_COBSR,          "Message encoding failure" )
_ERR_NAME( EMSG_TOO_LONG,       "Message too long" )
_ERR_NAME( EMSG_TOO_SHORT,      "Message too short" )
_ERR_NAME( EMSG_CRC,            "Message checksum failure" )
_ERR_NAME( EMSG_UNKNOWN,        "Unknown message" )

_ERR_GROUP( EBOOT_BASE, EUSER_BASE + 0x100 )
_ERR_NAME( EBOOT_INACTIVE,      "Bootloader not active" )
_ERR_NAME( EBOOT_FLASH,         "Flash error" )
_ERR_NAME( EBOOT_RANGE,         "Invalid address range" )
_ERR_NAME( EBOOT_CHECKSUM,      "Invalid checksum" )
_ERR_NAME( EBOOT_MAGIC,         "Invalid magic" )

_ERR_GROUP( EFLASH_BASE, EUSER_BASE + 0x200 )
_ERR_NAME( EFLASH_BUSY,         "EFLASH_BUSY" )
_ERR_NAME( EFLASH_RD,           "EFLASH_RD" )
_ERR_NAME( EFLASH_PGS,          "EFLASH_PGS" )
_ERR_NAME( EFLASH_PGP,          "EFLASH_PGP" )
_ERR_NAME( EFLASH_PGA,          "EFLASH_PGA" )
_ERR_NAME( EFLASH_WRP,          "EFLASH_WRP" )
_ERR_NAME( EFLASH_PROGRAM,      "EFLASH_PROGRAM" )
_ERR_NAME( EFLASH_OPERATION,    "EFLASH_OPERATION" )
_ERR_NAME( EFLASH_COMPLETE,     "EFLASH_COMPLETE" )

#ifndef _ERR_MAKE_STRING_TABLE
};

#undef _ERR_GROUP
#undef _ERR_NAME

// Translate FLASH_Status (see stm32f4xx_flash.h)
//
#define FLASH_Status_TO_ERRNO(x)    ((x) + EFLASH_BASE)

#ifdef __cplusplus
extern "C" {
#endif

const char *_user_strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif

#endif
