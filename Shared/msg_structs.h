#pragma once

/**
 * ATTENTION:
 *   This file is used by the DrQuad application as well as the
 *   Bootloader and QuadControl. Check all three if you make
 *   modifications.
 */

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

// XBee Series 1:           100 bytes
// XBee Series 2 ZNet:      72 bytes
// XBee Series 2 ZB Pro:    84 bytes
//
#define MSG_MAX_DATA_SIZE   (100 - 2 - 2)   // -crc -id

// TK TODO!
#define CHECK_MSG_LEN(t, m) \
    (sizeof(struct t) == (m).h.len)


enum msg_id {
    MSG_ID_NOP                  = 0x0000,

    MSG_ID_IMU_DATA             = 0x0010,

    MSG_ID_BOOT_ENTER           = 0xB000,
    MSG_ID_BOOT_READ_DATA       = 0xB001,
    MSG_ID_BOOT_VERIFY          = 0xB002,
    MSG_ID_BOOT_WRITE_DATA      = 0xB003,
    MSG_ID_BOOT_ERASE_SECTOR    = 0xB004,
    MSG_IG_BOOT_EXIT            = 0xB00F,
    MSG_ID_BOOT_RESPONSE        = 0xB010,

    MSG_ID_SHELL_TO_PC          = 0xC000,
    MSG_ID_SHELL_FROM_PC        = 0xC001
};


#pragma pack(push, 1)

/**
 * Message header
 *
 * The crc and id field are directly in front of the payload data
 * and are used during packet reception and transmission.
 *
 */
struct msg_header {
    int         data_len;       //  length of payload data

    // fields below are transmitted in the packet
    //
    uint16_t    crc;            //  message crc
    uint16_t    id;             //  message id
    uint8_t     data[/* data_len */];
};


static_assert(
    offsetof(struct msg_header, crc) % 4 == 0,
    "message fields must be aligned to 4 bytes"
);


struct msg_generic {
    struct msg_header h;
    uint8_t     data[MSG_MAX_DATA_SIZE];
};


/**
 * NOP packet (can be used as a keep-alive)
 * \todo -> Change to PING
 */
struct msg_nop
{
    struct msg_header h;
};


struct msg_imu_data
{
    struct msg_header h;
    float   acc_x, acc_y, acc_z;
    float   gyro_x, gyro_y, gyro_z;
    float   mag_x, mag_y, mag_z;
    float   baro_hpa, baro_temp;
};



/**
 * Enter bootloader
 */

#define BOOT_ENTER_MAGIC    0xB00710AD

struct msg_boot_enter
{
    struct msg_header h;
    uint32_t    magic;
};


/**
 * Read flash memory
 */
struct msg_boot_read_data
{
    struct msg_header h;
    uint32_t    address;
    uint8_t     length;
};


/**
 * Write flash memory
 */
struct msg_boot_write_data
{
    struct msg_header h;
    uint32_t    address;
    uint8_t     data[MSG_MAX_DATA_SIZE - 4];
};


/**
 * Verify flash memory
 */
struct msg_boot_verify
{
    struct msg_header h;
    uint32_t    address;
    uint32_t    length;
};


/**
 * Erase flash sector
 */
struct msg_boot_erase_sector
{
    struct msg_header h;
    uint8_t     sector;
};


/**
 * Exit bootloader
 */
struct msg_boot_exit
{
    struct msg_header h;
};


/**
 * Shell connection Vehicle -> PC
 */
struct msg_shell_to_pc
{
    struct msg_header h;
    uint8_t data[MSG_MAX_DATA_SIZE];
};


/**
 * Shell connection PC -> Vehicle
 */
struct msg_shell_from_pc
{
    struct msg_header h;
    uint8_t data[MSG_MAX_DATA_SIZE];
};


/**
 * Bootloader command response
 */
struct msg_boot_response
{
    struct msg_header h;
    int     error;
    uint8_t data[MSG_MAX_DATA_SIZE - 4];
};


#pragma pack(pop)
