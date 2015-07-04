#include "term_foo.h"

#include "term_xbee.h"
#include "command.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


static ssize_t foo_write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    // fwrite -> _fwrite_r -> __sfvwrite_r ->  fp->_write -> __swrite -> write_r
    //

    // send bytes as a shell packet
    //

    return 0;
}


static ssize_t foo_read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    return 0;
}


static ssize_t foo_chars_avail_r(struct _reent *r, int fd)
{
    return 0;
}


void term_foo_update(void)
{

}


void term_foo_init(void)
{
    dev_register("foo", &term_foo_ops);
}


struct file_ops term_foo_ops = {
    .read_r        = foo_read_r,
    .write_r       = foo_write_r,
    .chars_avail_r = foo_chars_avail_r
};

