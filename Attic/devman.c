/**
 * A simple device manager for newlib
 * Copyright (c)2010 Thomas Kindler <mail@t-kindler.de>
 *
 * This program is free software;  you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License,  or (at your option) any later version.  Read the
 * full License at http://www.gnu.org/copyleft for more details.
 */

// Include files -----
//
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <reent.h>
#include <errno.h>
#include <stdlib.h>
#include "devman.h"

#define DM_MAX_DEVICES  16
#define DM_MAX_HANDLES  16

struct dm_handle {
    const struct dm_device *dev;
    void  *info;
};

static const struct dm_device *dm_devs[DM_MAX_DEVICES];
static struct dm_handle        dm_handles[16];


/**
 * Register a device.
 *
 */
int dm_register_dev(const struct dm_device *d)
{
    for (int i=0; i<DM_MAX_DEVICES; i++) {
        if (!dm_devs[i]) {
            dm_devs[i] = d;
            return i;
        }
    }
    errno = ENOMEM;
    return -1;
}


/**
 * Look in the device manager table and find the device for the given name.
 * Returns an a device structure, NULL if error.
 *
 * Also returns a pointer to the actual file name (without the device part)
 *
 */
const struct dm_device *dm_find_dev(const char *path, const char **filename)
{
    if (!path || path[0] != '/') {
        errno = EINVAL;
        return NULL;
    }

    *filename = strchr(&path[1], '/');
    if (!*filename)
        *filename = &path[strlen(path)];


    for (int i=0; i<DM_MAX_DEVICES; i++) {
        const struct dm_device *d = dm_devs[i];

        if ( (strlen(d->name) == *filename - path - 1)  &&
             !strncmp(d->name, &path[1], *filename - path - 1)) {
            return d;
        }
    }

    errno = ENODEV;
    return NULL;
}


int  dm_alloc_handle(const struct dm_device *dev, void *info)
{
    for (int i=0; i<DM_MAX_HANDLES; i++) {
        if (!dm_handles[i].dev) {
            dm_handles[i].dev  = dev;
            dm_handles[i].info = info;
            return i;
        }
    }
    errno = ENFILE;
    return -1;
}


int  dm_free_handle(int fd)
{
    if (fd < 0 || fd >= DM_MAX_HANDLES || !dm_handles[fd].dev) {
        errno = EBADF;
        return -1;
    }
    else {
        dm_handles[fd].dev  = NULL;
        dm_handles[fd].info = NULL;
        return 0;
    }
}


// -------------------- Shell commands --------------------
//
#include "command.h"
#include "usb_serial.h"
#include "uart.h"

const struct dm_device dev_usb = {
    .name = "usb",
    .write_r = vcom_write_r,
    .read_r  = vcom_read_r
};

const struct dm_device dev_uart1 = {
    .name = "uart1",
    .write_r = uart_write_r,
    .read_r  = uart_read_r
};

const struct dm_device dev_uart2 = {
    .name = "uart2"
};

const struct dm_device dev_mmc = {
    .name = "mmc"
};


void cmd_open(int argc, char *argv[])
{
    if (!dm_devs[0]) {
        dm_register_dev(&dev_usb);
        dm_register_dev(&dev_uart1);
        dm_register_dev(&dev_uart2);
        dm_register_dev(&dev_mmc);

        // The first three file handles are std{in, out, err}
        //
        dm_alloc_handle(&dev_usb, NULL);
        dm_alloc_handle(&dev_usb, NULL);
        dm_alloc_handle(&dev_usb, NULL);
    }

    if (argc != 2) {
        printf("pööööö\n");
        return;
    }

    const struct dm_device *dev = NULL;
    const char *fn = NULL;

    dev = dm_find_dev( argv[1], &fn );
    printf("dev=%p \"%s\", fn=\"%s\"\n",
        dev, dev ? dev->name : "NULL",
        fn ? fn : "NULL"
    );
}


static void cmd_devs(void)
{
    for (int i=0; i<DM_MAX_DEVICES; i++) {
        const struct dm_device *d = dm_devs[i];
        if (d) {
            printf("%d: %s\n", i, d->name);
        }
    }
}


static void cmd_lsof(void)
{
    for (int i=0; i<DM_MAX_HANDLES; i++) {
        const struct dm_handle *h = &dm_handles[i];
        if (h->dev) {
            printf("%d: %s, %p\n", i, h->dev->name, h->info);
        }
    }
}

SHELL_CMD(open, (cmdfunc_t)cmd_open, "file open test");
SHELL_CMD(devs, (cmdfunc_t)cmd_devs, "show devices");
SHELL_CMD(lsof, (cmdfunc_t)cmd_lsof, "show open files");

