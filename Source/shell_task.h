#pragma once

struct shell_params {
    int fd_stdin;
    int fd_stdout;
    int fd_stderr;
};

void shell_task(void *pvParameters);

