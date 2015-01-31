#ifndef ERRORS_H
#define ERRORS_H

#define FLASH_Status_TO_ERRNO(x)    ((x) + EFLASH_BASE)

#ifndef MAKE_ERROR_TABLE
    #include <errno.h>

    enum {
        EUSER_BASE = __ELASTERROR,
        #define ENUM(name, desc)    name,
#endif


ENUM( EMSG_TIMEOUT,     "Message timed out" )
ENUM( EMSG_COBSR,       "Message encoding failure" )
ENUM( EMSG_TOO_LONG,    "Message too long" )
ENUM( EMSG_TOO_SHORT,   "Message too short" )
ENUM( EMSG_CRC,         "Message checksum failure" )
ENUM( EMSG_UNKNOWN,     "Unknown message" )

// ENUM( EBOOT_BASE = EUSER_BASE + 0x100, "-" )
ENUM( EBOOT_INACTIVE,   "Bootloader not active" )
ENUM( EBOOT_FLASH,      "Flash error" )
ENUM( EBOOT_RANGE,      "Invalid address range" )
ENUM( EBOOT_CHECKSUM,   "Invalid checksum" )
ENUM( EBOOT_MAGIC,      "Invalid magic" )

// Translated FLASH_Status from stm32f4xx_flash.h
//
// ENUM( EFLASH_BASE = EUSER_BASE + 0x200, "-" )

ENUM( EFLASH_BASE,      "-" )
ENUM( EFLASH_BUSY,      "EFLASH_BUSY" )
ENUM( EFLASH_RD,        "EFLASH_RD" )
ENUM( EFLASH_PGS,       "EFLASH_PGS" )
ENUM( EFLASH_PGP,       "EFLASH_PGP" )
ENUM( EFLASH_PGA,       "EFLASH_PGA" )
ENUM( EFLASH_WRP,       "EFLASH_WRP" )
ENUM( EFLASH_PROGRAM,   "EFLASH_PROGRAM" )
ENUM( EFLASH_OPERATION, "EFLASH_OPERATION" )
ENUM( EFLASH_COMPLETE,  "EFLASH_COMPLETE" )


#ifndef MAKE_ERROR_TABLE
};
#endif

#undef ENUM

#endif
