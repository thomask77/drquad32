#include "syscalls.h"
#include "term_xbee.h"
#include "term_usb.h"
#include "board.h"
#include "util.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <reent.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "stm32f4xx.h"

size_t   __malloc_margin = 256;
char    *__brkval;


struct device_info {
    const char *name;
    const struct file_ops *ops;
};

struct device_info  device_table[16];


int dev_register(const char *name, const struct file_ops *ops)
{
    for (int i=0; i<ARRAY_SIZE(device_table); i++) {
        if (!device_table[i].name) {
            device_table[i].name = name;
            device_table[i].ops  = ops;
            return i;
        }
    }

    errno = ENOMEM;
    return -1;
}


void _exit(int code)
{
    __BKPT(0);
    for(;;);
}

// Das hier ist alles ein ziemlicher Hack..
//
// TODO: Richtiges open() inkl. file-descriptor-table in
//   syscalls.c implementieren (siehe z.B. eLUA).
//
// TODO: freopen() bzw. dup2() verwenden, um streams zu redirecten
//
ssize_t _read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    int device = fd & 0xF000;
    switch (device) {
    case FD_DEV_XBEE:   return term_xbee_ops.read_r(r, fd, ptr, len);
    case FD_DEV_USB:    return term_usb_ops.read_r(r, fd, ptr, len);
    default:  r->_errno = ENOSYS;  return -1;
    }
}


ssize_t _write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{
    int device = fd & 0xF000;
    switch (device) {
    case FD_DEV_XBEE:   return term_xbee_ops.write_r(r, fd, ptr, len);
    case FD_DEV_USB:    return term_usb_ops.write_r(r, fd, ptr, len);
    default: r->_errno = ENOSYS;  return -1;
    }
}


ssize_t stdin_chars_avail(void)
{
    int fd = stdin->_file;
    int device = fd & 0xF000;
    switch (device) {
    case FD_DEV_XBEE:  return term_xbee_ops.chars_avail_r(_impure_ptr, fd);
    case FD_DEV_USB:   return term_usb_ops.chars_avail_r(_impure_ptr, fd);
    default: errno = ENOSYS;  return -1;
    }
}


off_t _lseek_r(struct _reent *r, int fd, _off_t ptr, int dir)
{
    fd  = fd;
    ptr = ptr;
    dir = dir;
    r->_errno = ENOSYS;
    return -1;
}


int _close_r(struct _reent *r, int fd)
{
    fd = fd;
    r->_errno = ENOSYS;
    return -1;
}


int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    st->st_mode = S_IFCHR;   // Always pretend to be a tty
    st->st_blksize = 0;
    return 0;
}


int _isatty_r(struct _reent *r, int fd)
{
    r  = r;
    fd = fd;
    return 1;
}


void *_sbrk_r(struct _reent *r, ptrdiff_t incr)
{
    extern char end;   // provided by the linker script

    if (__brkval == 0)
        __brkval = &end;

    if (__brkval + incr > (char*)__get_MSP() - __malloc_margin) {
        r->_errno = ENOMEM;
        return (void*)-1;
    }

    void *ret = __brkval;
    __brkval += incr;

    return ret;
}


int _kill_r(struct _reent *r, int pid, int signal)
{
    r->_errno = ENOSYS;
    return -1;
}


int _getpid_r(struct _reent *r)
{
    r->_errno = ENOSYS;
    return -1;
}


void __malloc_lock(struct _reent *r)
{
    vTaskSuspendAll();
}


void __malloc_unlock(struct _reent *r)
{
    xTaskResumeAll();
}


void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
    taskDISABLE_INTERRUPTS();
    board_set_leds(LED_RED);

    xbee_poll_send("assertion \"");
    xbee_poll_send(failedexpr);
    xbee_poll_send("\" failed: file \"");
    xbee_poll_send(file);
    xbee_poll_send("\", line ");

    char line_s[16];
    sprintf(line_s, "%d", line);
    xbee_poll_send(line_s);

    if (func) {
        xbee_poll_send(", function ");
        xbee_poll_send(func);
    }

    __BKPT(1);
    for(;;);
}


void vApplicationStackOverflowHook(TaskHandle_t *pxTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    board_set_leds(LED_RED);

    xbee_poll_send("Stack overflow in ");
    xbee_poll_send(pcTaskName);
    xbee_poll_send("\n");

    __BKPT(2);
    for(;;);
}
