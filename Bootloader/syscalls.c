#include "syscalls.h"
#include "stm32f4xx.h"
#include <stdlib.h>
#include <errno.h>

size_t   __malloc_margin = 256;
char    *__brkval;


__attribute__((used))
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
