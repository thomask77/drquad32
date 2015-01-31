#include "board.h"
#include "uart.h"
#include "crc32_sm.h"
#include "msg_structs.h"
#include "msg_packet.h"
#include "stm32f4xx.h"
#include "version.h"
#include "errors.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

#define XBEE_BAUDRATE       115200
#define BOOT_TIMEOUT        2000        // [ms]

#define APP_START           0x08010000
#define APP_END             0x08100000

#define APP_RANGE_VALID(a, s)   \
    (!(((a) | (s)) & 3) && (a) >= APP_START && ((a) + (s)) <= APP_END)

#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))

static volatile const struct version_info *app_info =  \
    (void*)(APP_START + VERSION_INFO_OFFSET);


#define DBG_PRINTF(...)
// #define DBG_PRINTF  msg_printf


static void toggle_led(void)
{
    static int toggle;

    if (toggle)
        board_set_leds(LED_RED | LED_GREEN);
    else
        board_set_leds(LED_RED);

    toggle = !toggle;
}


static bool check_empty(uint32_t addr, uint32_t size)
{
    for (int i=0; i<size; i+=4) {
        if (*(uint32_t*)addr != 0xFFFFFFFF)
            return false;
        addr += 4;
    }
    return true;
}


static int send_response(const void *buf, size_t len)
{
    struct msg_boot_response msg;

    if (len > sizeof(msg.data))
        len = sizeof(msg.data);

    msg.h.id = MSG_ID_BOOT_RESPONSE;
    msg.h.data_len = len;
    memcpy(msg.data, buf, len);

    msg_send(&msg.h);
    return len;
}


static int send_shell_to_pc(const char *buf, size_t len)
{
    struct msg_shell_to_pc msg;

    if (len > sizeof(msg.data))
        len = sizeof(msg.data);

    msg.h.id = MSG_ID_SHELL_TO_PC,
    msg.h.data_len = len;
    memcpy(msg.data, buf, len);

    msg_send(&msg.h);
    return len;
}


__attribute__((format(printf, 1, 2)))
int msg_printf(const char *fmt, ...)
{
    char buf[256];

    va_list args;
    va_start(args, fmt);

    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > sizeof(buf)-1) {
        // message was truncated
        //
        n = sizeof(buf)-1;
    }

    va_end(args);

    int todo = n;
    while (todo > 0)
        todo -= send_shell_to_pc(&buf[n-todo], todo);

    return n;
}


/*static*/ bool set_option_bytes(void)
{
    if (FLASH_OB_GetWRP() == 0xFFC  &&
        FLASH_OB_GetBOR() == OB_BOR_LEVEL3)
        return true;

    msg_printf("Setting option bytes\n ");

    FLASH_OB_Unlock();
    FLASH_OB_WRPConfig(OB_WRP_Sector_All, DISABLE);
    FLASH_OB_WRPConfig(OB_WRP_Sector_0,   ENABLE);
    FLASH_OB_WRPConfig(OB_WRP_Sector_1,   ENABLE);
    FLASH_OB_BORConfig(OB_BOR_LEVEL3);
    FLASH_OB_Launch();
    FLASH_OB_Lock();

    int res = FLASH_GetStatus();
    if (res != FLASH_COMPLETE) {
        errno = FLASH_Status_TO_ERRNO(res);
        return false;
    }

    return true;
}


static bool check_app(void)
{
    if (!APP_RANGE_VALID(APP_START, app_info->image_size)) {
        errno = EBOOT_RANGE;
        return false;
    }

    uint32_t crc = crc32_init();
    crc = crc32_update(crc, (void*)APP_START, app_info->image_size);
    crc = crc32_finalize(crc);

    if (crc != 0) {
        errno = EBOOT_CHECKSUM;
        return false;
    }

    return true;
}


static bool start_app(void)
{
    msg_printf("Starting application..\n");

    uint32_t  stack = ((const uint32_t *)APP_START)[0];
    uint32_t  entry = ((const uint32_t *)APP_START)[1];

    msg_printf(
        "  Initial stack: 0x%08lx\n"
        "  Entry point  : 0x%08lx\n"
        "  Image size   : %lu\n",
        stack, entry, app_info->image_size
    );

    if (!check_app())
        return false;

    // Point of no return
    //
    msg_printf("\n");
    uart_flush();

    board_set_leds(LED_RED | LED_GREEN);
    board_deinit();

    asm volatile(
        "msr    msp, %0        \n\t"
        "bx     %1             \n\t"
        : : "r" (stack), "r" (entry)
    );

    return true;
}


/********** Message handlers **********/

static int boot_active = 0;
static int boot_exit = 0;


static bool handle_boot_enter(const struct msg_boot_enter *msg)
{
    msg_printf("boot_enter(0x%08lx)\n", msg->magic);

    if (msg->magic != BOOT_ENTER_MAGIC) {
        errno = EBOOT_MAGIC;
        return false;
    }

    return true;
}


static bool handle_boot_read_data(const struct msg_boot_read_data *msg)
{
    DBG_PRINTF("boot_read_data(%04lx, %d)\n",
        msg->address, msg->length
    );

    send_response((void*)msg->address, msg->length);

    return true;
}


static bool handle_boot_verify(const struct msg_boot_verify *msg)
{
    msg_printf("boot_verify(0x%04lx, %lu): ",
        msg->address, msg->length
    );

    if (!APP_RANGE_VALID(msg->address, msg->length)) {
        send_response((uint32_t[]){ 0xDEADBEEF }, 4);
        errno = EBOOT_RANGE;
        return false;
    }

    crc32_t crc = crc32_init();
    crc = crc32_update(crc, (void*)msg->address, msg->length);
    crc = crc32_finalize(crc);

    send_response((uint32_t[]) { crc }, 4);

    msg_printf("0x%08lx\n", crc);

    return true;
}


static bool handle_boot_write_data(const struct msg_boot_write_data *msg)
{
    uint32_t addr = msg->address;
    uint32_t size = msg->h.data_len - 4;

    DBG_PRINTF("boot_write_data(0x%08lx, %lu)\n", addr, size);

    if (!APP_RANGE_VALID(addr, size)) {
        errno = EBOOT_RANGE;
        return false;
    }

    FLASH_Unlock();

    uint32_t *s = (uint32_t*)msg->data;

    for (int i=0; i < size; i+=4) {
        int res = FLASH_ProgramWord(addr + i, *s++);
        if (res != FLASH_COMPLETE) {
            errno = FLASH_Status_TO_ERRNO(res);
            FLASH_Lock();
            return false;
        }
    }

    FLASH_Lock();

    return true;
}


static bool handle_boot_erase_sector(const struct msg_boot_erase_sector *msg)
{
    msg_printf("boot_erase_sector(%d) ", msg->sector);

    static const struct { uint32_t  addr, size; } map[] = {
        { 0x08000000, 0x4000  }, { 0x08004000, 0x4000  },
        { 0x08008000, 0x4000  }, { 0x0800C000, 0x4000  },
        { 0x08010000, 0x10000 }, { 0x08020000, 0x20000 },
        { 0x08040000, 0x20000 }, { 0x08060000, 0x20000 },
        { 0x08080000, 0x20000 }, { 0x080A0000, 0x20000 },
        { 0x080C0000, 0x20000 }, { 0x080E0000, 0x20000 }
    };

    if (msg->sector >= ARRAY_SIZE(map) ||
        !APP_RANGE_VALID(map[msg->sector].addr, map[msg->sector].size) )
    {
        errno = EBOOT_RANGE;
        return false;
    }

    if (check_empty(map[msg->sector].addr, map[msg->sector].size)) {
        msg_printf("already empty\n");
    }
    else {
        FLASH_Unlock();
        int res = FLASH_EraseSector(msg->sector << 3, VoltageRange_3);
        FLASH_Lock();

        if (res != FLASH_COMPLETE) {
            errno = FLASH_Status_TO_ERRNO(res);
            return false;
        }
        msg_printf("ok\n");
    }

    return true;
}


static bool handle_boot_exit(const struct msg_boot_exit *msg)
{
    msg_printf("boot_exit()\n");
    boot_active = false;
    boot_exit = true;
    return true;
}


static bool handle_boot_unknown(const struct msg_generic *msg)
{
    msg_printf("unknown message(0x%04x, %d)\n", msg->h.id, msg->h.data_len);
    errno = EMSG_UNKNOWN;
    return false;
}


static bool handle_message(const struct msg_generic *msg)
{
    if (msg->h.id == MSG_ID_BOOT_ENTER) {
        boot_active = handle_boot_enter((void*)msg);
        return true;
    }

    if (!boot_active) {
        errno = EBOOT_INACTIVE;
        return false;
    }

    switch (msg->h.id) {
    case MSG_ID_BOOT_READ_DATA:     return handle_boot_read_data((void*)msg);
    case MSG_ID_BOOT_VERIFY:        return handle_boot_verify((void*)msg);
    case MSG_ID_BOOT_WRITE_DATA:    return handle_boot_write_data((void*)msg);
    case MSG_ID_BOOT_ERASE_SECTOR:  return handle_boot_erase_sector((void*)msg);
    case MSG_IG_BOOT_EXIT:          return handle_boot_exit((void*)msg);
    default:                        return handle_boot_unknown((void*)msg);
    }
}


static void message_loop(void)
{
    msg_printf("Waiting for commands.. ");
    uint32_t t_last_msg = tickcount;

    while (!boot_exit) {
        if (tickcount - t_last_msg > BOOT_TIMEOUT) {
            msg_printf("\n  Timed out\n");
            return;
        }

        struct msg_generic msg;
        int len = msg_recv(&msg.h);

        if (len >= 0) {
            toggle_led();

            bool res = handle_message(&msg);
            if (!res)
                msg_printf("  %s\n", strerror(errno));

            send_response((uint8_t[]){ res }, 1);

            t_last_msg = tickcount;
        }
        else {
            if (errno == EMSG_TIMEOUT)
                msg_printf(".");
            else
                msg_printf("  %s\n", strerror(errno));
        }
    }
}


int main(void)
{
    board_init();
    board_set_leds(LED_RED);
    uart_init(XBEE_BAUDRATE);

    msg_printf("%s", ""); // flush messages
    msg_printf(
        "\n\n"
        "\033[1;36m%s\033[0;39m v%d.%d.%d %s %s %s\n"
        "Copyright (c)2015 Thomas Kindler <mail@t-kindler.de>\n"
        "\n",
        version_info.product_name, version_info.major,
        version_info.minor, version_info.patch,
        version_info.git_version,
        version_info.build_date, version_info.build_time
    );

    // Sigh.  Flash loader demonstrator 2.5.0
    // and OpenOCD 0.6.1 can't unlock the flash m(
    //
    // set_option_bytes();
    //

    message_loop();

    if (!start_app())
        msg_printf("  %s\n", strerror(errno));

    uart_flush();
    NVIC_SystemReset();
}
