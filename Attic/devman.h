#ifndef DEVMAN_H
#define DEVMAN_H

#include <stdint.h>

// A device structure with pointers to all the device functions
//
struct dm_device {
    const char *name;

    int      (*open_r)  (struct _reent *r, const char *path, int flags, int mode);
    int      (*close_r) (struct _reent *r, int fd);

    ssize_t  (*write_r) (struct _reent *r, int fd, const void *ptr, size_t len);
    ssize_t  (*read_r)  (struct _reent *r, int fd, void *ptr, size_t len);

    off_t    (*lseek_r) (struct _reent *r, int fd, off_t off, int whence);
};


extern  int    dm_register_dev(const struct dm_device *d);

extern  const struct dm_device
        *dm_find_dev(const char *path, const char **filename);

#endif
