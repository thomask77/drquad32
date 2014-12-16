#include "spi_flash.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Chip select pin
//
#define  DF_CS_GPIO         GPIOB
#define  DF_CS_PIN          GPIO_Pin_8

// Command codes
//
#define  PROG_PAGE          0x02
#define  READ_ARRAY         0x03
#define  READ_STATUS        0x05
#define  WRITE_ENABLE       0x06
#define  ERASE_SECTOR       0x20
#define  ERASE_CHIP         0xC7
#define  READ_JEDEC_ID      0x9F

// Status register bits
//
#define  STAT_BUSY          0x01

#define  PAGE_ADDR(a)       (((a) / df_info->page_size)   * df_info->page_size  )
#define  SECTOR_ADDR(a)     (((a) / df_info->sector_size) * df_info->sector_size)

// Buffer memory for one erase unit
//
#define  BUFFER_SIZE        4096

// #define  DF_DEBUG           printf
#define  DF_DEBUG(...)  /* disabled */

static const struct df_info  winbond_w25q16 = {
    .name        = "Winbond W25Q16",
    .jedec_id    = 0xEF4015,
    .chip_size   = 2 * 1024 * 1024,
    .page_size   = 256,
    .sector_size = 4096
};


const struct df_info *df_info = NULL;

static char       buf_data[BUFFER_SIZE];
static uint32_t   buf_addr  = -1;
static bool       buf_dirty = false;


static int read_status(void)
{
    uint8_t cmd[] = { READ_STATUS, 0 };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, cmd, sizeof cmd);
    spi_end();
    return cmd[1];
}

/**
 * Wait until the data flash is ready.
 *
 * The actual execution time ranges from a few milliseconds
 * for a page write up to half a minute for a full chip erase.
 * We use a worst-case timeout of 60s for all operations,
 * because we have caught some serious error anyways.
 *
 * \return  status register value or <0 if there was an error
 */
static int wait_ready(void)
{
    for (int t=0; t < 60000 / portTICK_RATE_MS; t++) {
        int status = read_status();
        if (!(status & STAT_BUSY))
            return status;
        vTaskDelay(1);
    }

    DF_DEBUG("dataflash timeout!\n");
    errno = ETIMEDOUT;
    return -1;
}


static uint32_t read_jedec_id(void)
{
    uint8_t cmd[] = { READ_JEDEC_ID, 0, 0, 0 };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, cmd, sizeof cmd);
    spi_end();

    return (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
}


static void write_enable(void)
{
    uint8_t cmd[] = { WRITE_ENABLE };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, NULL, sizeof cmd);
    spi_end();
}


static int erase_sector(uint32_t addr)
{
    DF_DEBUG("erase_sector(%lu)\n", addr);
    assert(addr == SECTOR_ADDR(addr));

    write_enable();

    uint8_t cmd[] = { ERASE_SECTOR, addr>>16, addr>>8, addr };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, NULL, sizeof cmd);
    spi_end();

    return wait_ready();
}


static int write_page(uint32_t addr, const void *data)
{
    DF_DEBUG("write_page(%lu, %p)\n", addr, data);
    assert(addr == PAGE_ADDR(addr));

    write_enable();

    uint8_t cmd[] = { PROG_PAGE, addr>>16, addr>>8, addr };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, NULL, sizeof cmd);
    spi_transfer(data, NULL, df_info->page_size);
    spi_end();

    return wait_ready();
}


static int read_sector(uint32_t addr)
{
    DF_DEBUG("read_sector(%lu) ", addr);
    assert(addr == SECTOR_ADDR(addr));

    if (buf_addr == addr) {
        DF_DEBUG("hit\n");
        return df_info->sector_size;
    }
    DF_DEBUG("miss\n");

    if (buf_dirty) {
        if (df_flush() < 0)
            return -1;
    }

    uint8_t cmd[] = { READ_ARRAY, addr>>16, addr>>8, addr };

    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, NULL, sizeof cmd);
    spi_transfer(NULL, buf_data, df_info->sector_size);
    spi_end();

    buf_addr = addr;

    return df_info->sector_size;
}


/**
 * Read from serial flash memory.
 *
 * \param      addr  flash address
 * \param[out] data  data buffer
 * \param      len   number of bytes to read
 *
 * \returns   number of bytes read or -1 if there was an error.
 */
int df_read(uint32_t addr, void *data, uint32_t len)
{
    DF_DEBUG("df_read(%lu, %p, %lu)\n", addr, data, len);
    assert(df_info != NULL);

    if (len + addr > df_info->chip_size)
        len = df_info->chip_size - addr;

    uint32_t  todo = len;
    while (todo > 0) {
        int offset = addr - SECTOR_ADDR(addr);
        int chunk  = df_info->sector_size - offset;

        if (chunk > todo)
            chunk = todo;

        if (read_sector(SECTOR_ADDR(addr)) < 0)
            return -1;

        memcpy(data, &buf_data[offset], chunk);
        addr += chunk;
        data += chunk;
        todo -= chunk;
    }

    return len;
}


/**
 * Write to serial flash memory.
 *
 * \note  Writes are buffered to allow random access while
 *        minimizing sector erase commands. Use df_flush()
 *        to commit changes to flash memory.
 *
 * \param      addr  flash address
 * \param[in] data  data buffer
 * \param      len   number of bytes to write
 *
 * \returns    number of bytes written or <0 if there was an error.
 */
int df_write(uint32_t addr, const void *data, uint32_t len)
{
    DF_DEBUG("df_write(%lu, %p, %lu)\n", addr, data, len);
    assert(df_info != NULL);

    if (len + addr > df_info->chip_size)
        len = df_info->chip_size - addr;

    uint32_t  todo = len;
    while (todo > 0) {
        int offset = addr - SECTOR_ADDR(addr);
        int chunk  = df_info->sector_size - offset;

        if (chunk > todo)
            chunk = todo;

        // read_sector will also flush if necessary.
        //
        if (read_sector(SECTOR_ADDR(addr)) < 0)
            return -1;

        memcpy(&buf_data[offset], data, chunk);
        addr += chunk;
        data += chunk;
        todo -= chunk;
        buf_dirty = true;
    }

    return len;
}


/**
 * Commit outstanding writes to flash memory.
 *
 * \return  number of bytes written or <0 if there was an error.
 */
int df_flush(void)
{
    DF_DEBUG("df_flush() ");
    assert(df_info != NULL);

    if (!buf_dirty) {
        DF_DEBUG("clean\n");
        return 0;
    }

    DF_DEBUG("dirty\n");

    if (erase_sector(buf_addr) < 0)
        return -1;

    char *src = buf_data;
    int   dst = buf_addr;

    for (int i=0; i < df_info->sector_size/df_info->page_size; i++) {
        if (write_page(dst, src) < 0)
            return -1;

        src += df_info->page_size;
        dst += df_info->page_size;
    }
    buf_dirty = false;

    return df_info->sector_size;
}

/**
 * Erase the entire chip.
 *
 * \return  number of bytes erased or <0 if there was an error.
 */
int df_erase_chip(void)
{
    assert(df_info != NULL);

    buf_dirty = false;
    buf_addr  = -1;

    uint8_t cmd[] = { ERASE_CHIP };

    write_enable();
    spi_begin(DF_CS_GPIO, DF_CS_PIN);
    spi_transfer(cmd, NULL, sizeof cmd);
    spi_end();

    if (wait_ready() < 0)
        return -1;

    return df_info->chip_size;
}


/**
 * Initialize the serial flash driver.
 *
 */
bool df_init(void)
{
    // Initialize the chip select output
    //
    RCC->AHB1ENR |= RCC_AHB1Periph_GPIOB;
    GPIO_Init(DF_CS_GPIO, &(GPIO_InitTypeDef) {
        .GPIO_Pin   = DF_CS_PIN,
        .GPIO_Mode  = GPIO_Mode_OUT,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_OType = GPIO_OType_PP
    });

    uint32_t jedec_id = read_jedec_id();

    if (jedec_id == winbond_w25q16.jedec_id) {
        df_info = &winbond_w25q16;
    }
    else {
        printf("unknown serial flash 0x%08lx.\n", jedec_id);
        errno = ENOTSUP;
        return false;
    }

    printf("%s: %lu kbytes in %lu byte sectors.\n",
        df_info->name, df_info->chip_size >> 10, df_info->sector_size
    );

    assert(df_info && df_info->sector_size <= BUFFER_SIZE);

    return true;
}

// -------------------- Shell commands --------------------
//
#include "command.h"
#include "util.h"

static void cmd_df_read(int argc, char *argv[])
{
    char page_buf[df_info->page_size];

    if (argc != 2)
        goto usage;

    char    *endp;

    uint32_t  addr = strtoul(argv[1], &endp, 0);

    if (*endp)
        goto usage;

    df_read(addr, page_buf, df_info->page_size);
    hexdump(page_buf, df_info->page_size);
    return;

usage:
    printf("Usage: %s <addr>\n", argv[0]);
}


SHELL_CMD(df_erase, (cmdfunc_t)df_erase_chip, "erase dataflash" )
SHELL_CMD(df_read,  (cmdfunc_t)cmd_df_read,   "read dataflash"  )


#if 0

static void cmd_df_write(int argc, char *argv[])
{
    if (argc != 4)
        goto usage;

    char *endp;
    uint32_t addr = strtoul(argv[1], &endp, 0);
    if (*endp)
        goto usage;

    uint32_t src = strtoul(argv[2], &endp, 0);
    if (*endp)
        goto usage;

    uint32_t len = strtoul(argv[3], &endp, 0);
    if (*endp)
        goto usage;

    df_write(addr, (void*)src, len);
    df_flush();
    return;

usage:
    printf("Usage: %s <addr> <src> <len>\n", argv[0]);
}


static void cmd_df_test()
{
    const char *testdata = (const char *)0x8000000;

    printf("erasing chip.. ");
    df_erase_chip();

    // test single-sector ops
    //
    printf("\n* test: 2048 bytes at 0\n");
    df_write(   0, testdata, 2048);
    printf("\n* test: 2048 bytes at 1024\n");
    df_write(1024, testdata, 2048);
    printf("\n* test: 2048 bytes at 2048\n");
    df_write(2048, testdata, 2048);
    printf("\n* test: 4096 bytes at 0\n");
    df_write(   0, testdata, 4096);

    // test multi-sector ops
    //
    printf("\n* test: 8192 bytes at 0\n");
    df_write(   0, testdata, 8192);
    printf("\n* test: 12288 bytes at 0\n");
    df_write(   0, testdata, 12288);
    printf("\n* test: 12288 bytes at 256\n");
    df_write( 256, testdata, 12288);
    printf("\n* test: 12288 bytes at 3840\n");
    df_write( 3840, testdata, 12288);

    df_flush();
}

SHELL_CMD(df_test,  (cmdfunc_t)cmd_df_test,   "dataflash unit-test" )
SHELL_CMD(df_write, (cmdfunc_t)cmd_df_write,  "write dataflash" )

#endif
