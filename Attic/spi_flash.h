#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>

struct df_info {
    const char *name;
    uint32_t  jedec_id;
    uint32_t  chip_size;
    uint32_t  page_size;
    uint32_t  sector_size;
};

extern const struct df_info *df_info;

bool  df_init(void);
int   df_read(uint32_t addr, void *data, uint32_t len);
int   df_write(uint32_t addr, const void *data, uint32_t len);
int   df_flush(void);
int   df_erase_chip(void);

#endif
