#pragma once

#include <reent.h>
#include <sys/types.h>

#define FD_DEV_XBEE     0x0000
#define FD_DEV_USB      0x1000

struct file_ops {
    ssize_t (*read_r)       (struct _reent *r, int fd, void *ptr, size_t len);
    ssize_t (*write_r)      (struct _reent *r, int fd, const void *ptr, size_t len);
    ssize_t (*chars_avail_r)(struct _reent *r, int fd);

/*
    off_t   (*lseek_r)      (struct _reent *r, int fd, _off_t ptr, int dir);
    int     (*open_r)       (struct _reent *r, const char *pathname, int flags);
    int     (*close_r)      (struct _reent *r, int fd);
    int     (*fstat_r)      (struct _reent *r, int fd, struct stat *st);
    int     (*isatty_r)     (struct _reent *r, int fd);
*/
};

int dev_register(const char *name, const struct file_ops *ops);


ssize_t stdin_chars_avail(void);

