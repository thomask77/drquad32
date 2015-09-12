/**
 * Lightweight command parser for memory constrained systems
 *
 * Copyright (c)2003-2012 Thomas Kindler <mail@t-kindler.de>
 *
 * 2009-11-17: tk, added watch and help commands
 * 2009-09-11: tk, added command completion
 * 2009-08-31: tk, re-implemented in c with link-time command table
 * 2003-10-18: tk, initial implementation in c++
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "command.h"
#include "readline.h"
#include "ansi.h"
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/**
 * These symbols are provided by the linker at
 * the start and end of the command table section.
 */
extern const struct cmd_info __cmd_tbl_start[];
extern const struct cmd_info __cmd_tbl_end[];


/**
 * Find a command by name.
 *
 * \param    start  first command to start search at
 * \param    s      (partial) command name
 * \param    n      number of chars in s to match
 *
 * \return   pointer to matching command, or NULL
 */
static const struct cmd_info *cmd_find(const struct cmd_info *start, const char *s, int n)
{
    const struct cmd_info *cmd = start;

    while (cmd < __cmd_tbl_end) {
        if (!strncmp(cmd->name, s, n))
            return cmd;
        cmd++;
    }
    return NULL;
}


/**
 * Find the longest common prefix for command completion.
 *
 * \param   s   pointer to command name string
 * \param   n   number of chars in s to match
 *
 * \return  length of longest common prefix
 */
static int common_prefix(const char *s, int n)
{
    int i, len = strlen(s);

    const struct cmd_info *cmd = __cmd_tbl_start;

    while ((cmd = cmd_find(cmd, s, n))) {
        for (i = 0; i < len && cmd->name[i] == s[i]; i++)
            ;
        len = i;
        cmd++;
    }

    return len;
}


/**
 * Print a list of possible command completions.
 *
 * \param   s   (partial) command name
 * \param   n   number of chars in s to match
 */
static void print_completions(const char *s, int n)
{
    int count = 0;
    int maxlen = 0;

    const struct cmd_info *cmd = __cmd_tbl_start;

    // 1. find the maximum command name length
    //
    while ((cmd = cmd_find(cmd, s, n))) {
        int len = strlen(cmd->name);
        if (len > maxlen)
            maxlen = len;
        count++;
        cmd++;
    }

    // 2. construct a printf format string
    //
    char fmt[16];
    sprintf(fmt, "%%-%ds", maxlen + 2);

    int perline = TERMINAL_WIDTH / (maxlen + 2);

    // 3. print a nicely formatted table of commands
    //
    cmd = __cmd_tbl_start;
    while (cmd) {
        int i;
        for (i = 0; i < perline; i++) {
            cmd = cmd_find(cmd, s, n);
            if (!cmd)
                break;

            printf(fmt, cmd->name);
            cmd++;
        }
        if (i>0)
            printf("\n");
    }
}


/**
 * TAB-key shell command completion.
 *
 * \param[in,out]  line    command line to complete
 * \param          cursor  current cursor position
 * \param          n       maximum line length (including \\0)
 *
 * \return  new cursor position
 *
 * \todo  perhaps we should ignore leading and trailing
 *        white space when searching for a command.
 * \todo  (more general) implement a callback mechanism for
 *        command parameter completion.
 *
 */
int cmd_completion(char *line, int cursor, int n)
{
    const struct cmd_info *cmd1 = cmd_find(__cmd_tbl_start, line, cursor);
    if (cmd1 == NULL) {
        // no completion found
        //
        return cursor;
    }

    bool isUnique = cmd_find(cmd1 + 1, line, cursor) == NULL;
    if (!isUnique) {
        // more than one completion found
        //
        printf("\r" ANSI_ERASE_EOL);
        print_completions(line, cursor);
    }

    int prefix_len = common_prefix(cmd1->name, cursor);
    int line_len = strlen(line);

    int cmd_len = 0;
    while (line[cmd_len] && line[cmd_len] != ' ')
        cmd_len++;

    if (line_len - cmd_len + prefix_len + 2 > n)
        return cursor;

    // cut the existing command string
    //
    memmove(line, &line[cmd_len], line_len - cmd_len + 1);
    line_len -= cmd_len;

    // paste the new completion string
    //
    memmove(&line[prefix_len], line, line_len + prefix_len + 1);
    memmove(line, cmd1->name, prefix_len);
    line_len += prefix_len;

    // and add a trailing space if the command was unique
    //
    if (isUnique && !line[prefix_len]) {
        line[prefix_len++] = ' ';
        line[prefix_len] = 0;
    }

    return prefix_len;
}


/**
 * Build an argument vector from a string.
 *
 * Given a pointer to a string, parse the string extracting fields
 * separated by whitespace and optionally enclosed within either single
 * or double quotes (which are stripped off), and build a vector of
 * pointers for each field.  
 * 
 * \param[in,out] s   pointer to string to parse
 * \param   argv      pointer to argv array
 * \param   maxargc   maximum number of argv entries (including NULL)
 * \return  number of extracted arguments, or -1 if too many args.
 *
 * \note    This version of buildargv <i>does modify</i> the original
 *          string. All argv entries point to memory in the given
 *          string s without additional memory being allocated.
 */
static int buildargv(char *s, char *argv[], int maxargc)
{
    bool squote = false;
    bool dquote = false;
    bool bsquote = false;
    char *arg = s;
    int argc = 0;

    for (;;) {
        if (argc >= maxargc - 1) {
            errno = E2BIG;
            return -1;
        }

        while (isspace((int)*s))
            s++;

        if (!*s)
            break;

        arg = s;
        argv[argc++] = arg;

        while (*s) {
            if (bsquote) {
                *arg++ = *s;
                bsquote = false;
            }
            else if (*s == '\\') {
                bsquote = true;
            }
            else if (squote) {
                if (*s == '\'')
                    squote = false;
                else
                    *arg++ = *s;
            }
            else if (dquote) {
                if (*s == '"')
                    dquote = false;
                else
                    *arg++ = *s;
            }
            else {
                if (isspace((int)*s))
                    break;
                if (*s == '\'')
                    squote = true;
                else if (*s == '"')
                    dquote = true;
                else
                    *arg++ = *s;
            }
            s++;
        }
        if (!*s++)
            break;

        *arg = 0;
    }

    *arg = 0;
    argv[argc] = NULL;
    return argc;
}


/**
 * Execute a command.
 *
 * \param  argc  number of arguments in argv
 * \param  argv  pointer to argv array
 *
 * \return return code of the command or -1 in case of an error
 */
int cmd_exec(int argc, char *argv[])
{
    if (argc == 0)
        return 0;

    const struct cmd_info *cmd = cmd_find(__cmd_tbl_start, argv[0], INT_MAX);

    if (!cmd) {
        printf("%s: not found\n", argv[0]);
        errno = ENOENT;
        return -1;
    }

    return cmd->func(argc, argv);
}


/**
 * Parse and execute a command string.
 *
 * \param[in,out]   s     pointer to command string
 * \return  return code of the command
 *
 * \note    This function <i>does modify</i> the command string.
 */
int cmd_system(char *s)
{
    char *argv[MAX_ARGS];
    int argc = buildargv(s, argv, MAX_ARGS);

    if (argc < 0) {
        printf("too many arguments\n");
        errno = E2BIG;
        return -1;
    }

    // strip #-comments
    //
    for (int i=0; i<argc; i++) {
        if (argv[i][0] == '#') {
            argv[i] = NULL;
            argc = i;
            break;
        }
    }

    return cmd_exec(argc, argv);
}

// -------------------- Shell commands --------------------
//
/**
 * Print all commands and their short help texts.
 *
 */
static int cmd_help(int argc, char *argv[])
{
    (void)argc; (void)argv;

    const struct cmd_info *cmd = __cmd_tbl_start;

    while (cmd < __cmd_tbl_end) {
        printf("%-24s%s\n", cmd->name, cmd->help);
        cmd++;
    }

    return 0;
}

SHELL_CMD(help,  cmd_help,  "show commands")
