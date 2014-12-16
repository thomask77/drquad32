#pragma once

#define TERMINAL_WIDTH   80     ///< Terminal width in chars

// History buffer holding zero terminated lines
//
struct rl_history {
    char *buf;
    int  size;      ///< Size of the buffer in bytes
    int  len;       ///< First free byte in the buffer
    int  lines;     ///< Number of lines in the history
};

void hist_add(struct rl_history *h, const char *s);
int  readline(const char *prompt, char *s, int n, struct rl_history *h);

