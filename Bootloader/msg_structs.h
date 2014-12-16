#pragma once
#pragma pack(push, 1)


enum ids {
    MSG_ID_NOP                  = 0x0000,

    MSG_ID_BOOT_ENTER           = 0xB000,
    MSG_ID_BOOT_RESPONSE        = 0xB001,
    MSG_ID_BOOT_READ_DATA       = 0xB002,
    MSG_ID_BOOT_VERIFY          = 0xB003,
    MSG_ID_BOOT_WRITE_DATA      = 0xB004,
    MSG_ID_BOOT_ERASE_SECTOR    = 0xB005,
    MSG_ID_BOOT_EXIT            = 0xB00F,

    MSG_ID_SHELL_TO_PC          = 0xC000,
    MSG_ID_SHELL_FROM_PC        = 0xC001
};


/**
 * Message header
 *
 * The crc and id field are directly in front of the payload data
 * and are used during packet reception and transmission.
 *
 * Arbitrary data can be added before these fields (e.g. data_len,
 * timestamp, etc.), but care should be taken to maintain 4-byte
 * alignment for the actual payload.
 *
 */
struct msg_header {
    uint8_t     data_len;   //  length of payload data
    uint8_t     reserved;

    // fields below are transmitted in the packet
    //
    uint16_t    crc;        //  message crc
    uint16_t    id;         //  message id
};


struct msg_generic {
    struct msg_header h;
    uint8_t     data[255];
};


/**
 * NOP packet (can be used as a keep-alive)
 * \todo -> Change to PING
 */
struct msg_nop
{
    struct msg_header h;
};


/**
 * Enter bootloader
 */
struct msg_boot_enter
{
    struct msg_header h;
    uint32_t    magic;
};


/**
 * Bootloader command response
 */
struct msg_boot_response
{
    struct msg_header h;
    uint8_t     data[255];
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
    uint8_t     length;
    uint8_t     data[240];
};


/**
 * Verify flash memory
 */
struct msg_boot_verify_data
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
    uint8_t     data[255];
};


/**
 * Shell connection PC -> Vehicle
 */
struct msg_shell_from_pc
{
    struct msg_header h;
    uint8_t     data[255];
};


#pragma pack(pop)
