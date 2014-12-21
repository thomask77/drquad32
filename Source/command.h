#pragma once

#define MAX_ARGS    20

/**
 *  All command definitions go into their own code section.
 *
 */
#define SHELL_CMD(name, cmd, help)                          \
    const struct cmd_info __shell_cmd_##name                \
    __attribute__((used, section (".cmd_tbl."#name)))     \
        = {(#name), (cmd), (help)};

typedef   int (*cmdfunc_t)(int, char *[]);

struct cmd_info {
    const char *name;
    cmdfunc_t   func;
    const char *help;
};

int cmd_completion(char *line, int cursor, int n);

int cmd_exec(int argc, char *argv[]);
int cmd_system(char *s);
