#include "shell_task.h"
#include "command.h"
#include "readline.h"
#include "board.h"
#include "Shared/ansi.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "FreeRTOS.h"
#include "task.h"

/* following is copied from libc/stdio/local.h to check std streams */
extern void   _EXFUN(__sinit,(struct _reent *));
#define CHECK_INIT(ptr) \
    if ((ptr) && !(ptr)->__sdidinit) __sinit (ptr);


void shell_task(void *pvParameters)
{
    struct shell_params *params = pvParameters;

    if (params) {
        // Hack to redirect stdin/stdout
        // \todo replace by fdopen()
        //
        CHECK_INIT(_impure_ptr);
        stdin->_file  = params->fd_stdin;
        stdout->_file = params->fd_stdout;
        stderr->_file = params->fd_stderr;
        fflush(NULL);
    }

    struct rl_history  history = { 0 };
    history.size   = 256;
    history.buf    = malloc(history.size);
    history.buf[0] = 0;

    // save some useful commands..
    //
    hist_add(&history, "about");
    hist_add(&history, "gp :");
    hist_add(&history, "help");

    char prompt[16], line[80];
    sprintf(prompt, "> ");

    for(;;) {
        line[0] = '\0';
        int n = readline(prompt, line, sizeof line, &history);
        if (n < 0)
            break;

        cmd_system(line);
    }

    vTaskDelete(NULL);
}
