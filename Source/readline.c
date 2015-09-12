/**
 * Readline library for memory constrained systems
 *
 * Copyright (c)2001-2012 Thomas Kindler <mail@t-kindler.de>
 *
 * 2013-09-15: tk, readline now returns -1 on EOF
 * 2012-12-24: tk, state structure for history handling
 * 2009-09-02: tk, added history search
 * 2009-08-28: tk, in-place history buffer without dynamic memory
 * 2009-08-26: tk, added support for more terminals
 * 2002-08-04: tk, GNU readline-compatible editing commands
 * 2001-09-30: tk, initial implementation
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

#include "readline.h"
#include "command.h"
#include "ansi.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

// ANSI Escape sequences for special keys
//
#define KEY_UP          0x1B5B41
#define KEY_DOWN        0x1B5B42
#define KEY_RIGHT       0x1B5B43
#define KEY_LEFT        0x1B5B44
#define KEY_HOME1       0x1B5B317E  ///< sent by PuTTY
#define KEY_HOME2       0x1B4F48    ///< sent by GtkTerm
#define KEY_HOME3       0x1B5B48    ///< sent by HyperTerminal
#define KEY_INSERT      0x1B5B327E
#define KEY_DELETE      0x1B5B337E
#define KEY_END1        0x1B5B347E  ///< sent by PuTTY
#define KEY_END2        0x1B4F46    ///< sent by GtkTerm
#define KEY_END3        0x1B5B4B    ///< sent by HyperTerminal
#define KEY_PAGE_UP     0x1B5B357E
#define KEY_PAGE_DOWN   0x1B5B367E

#define CTRL(x)         (x-'@')

/**
 * Add a line to the history buffer.
 *
 * If the buffer is full, one or more old lines are deleted to make
 * room for the new one. If the string is longer than the history
 * buffer itself, the string will not be added.
 *
 * \param  s  string to add
 */
void hist_add(struct rl_history *h, const char *s)
{
    int len = strlen(s)+1;
    if (len > h->size)
        return;

    while (len + h->len > h->size) {
        int scrap = strlen(h->buf) + 1;
        memmove(h->buf, &h->buf[scrap], h->len - scrap);
        h->len -= scrap;
        h->lines--;
    }

    strcpy(&h->buf[h->len], s);
    h->len += len;
    h->lines++;
}


/**
 * Get a line from the history buffer.
 *
 * \param   line  number of line to get
 *
 * \return  pointer to string, or NULL if out of bounds
 */
static const char *hist_get(const struct rl_history *h, int line)
{
    const char *s = h->buf;
    for (int i=0; i < h->lines; i++) {
        if (i == line)
            return s;
        s += strlen(s)+1;
    }

    return NULL;
}


/**
 * Find a line in the history buffer.
 *
 * \param   first  index of first line
 * \param   s2     string to search
 * \param   n      number of characters to compare
 *
 * \return  index of first matching line, or -1 if not found
 */
static int hist_find(const struct rl_history *h, int first, const char *s2, int n)
{
    const char *s = h->buf;
    for (int i=0; i < h->lines; i++) {
        if (i >= first && !strncmp(s, s2, n))
            return i;
        s += strlen(s)+1;
    }

    return -1;
}


/**
 * Read a character or ANSI control sequence from stdin.
 *
 */
static int32_t getchar_ex(void)
{
    int32_t c, ret = 0;

    for (int i=0; i<4; i++) {
        if ((c = getchar()) == -1)
            return -1;
        ret = (ret << 8) | c;

        switch (i) {
        // Check for ESC
        //
        case 0: if (c != 0x1B) return ret; break;

        // Check for ANSI \e[.. or \eO..  escape sequences
        //
        case 1: if (c != 0x5B && c != 0x4F) return ret; break;

        // Read final bytes
        //
        case 2:
        case 3: if (c >= 0x40 && c <= 0x7e) return ret; break;
        }
    }
    return ret;
}


/**
 * A simple version of the GNU readline function.
 *
 * \param  prompt  command prompt
 * \param  s  pointer to string buffer
 * \param  n  maximum length (including \\0)
 * \param  h  pointer to a history structure
 *
 * \return length of string or -1 if EOF
 */
int readline(const char *prompt, char *s, int n, struct rl_history *h)
{
    int  len     = strlen(s);
    int  cursor  = len;
    int  reprint = true;
    int  hline   = h->lines;
    int  match, lastmatch;

    printf(ANSI_ENABLE_INSERT);

    for (;;) {
        if (reprint) {
            printf("\r" ANSI_ERASE_EOL "%s%s", prompt, s);
            if (cursor < len)
                printf(ANSI_LEFTn(%d), len-cursor);

            reprint = false;
        }
        fflush(stdout);

        int32_t c = getchar_ex();
        if (c == EOF)
            return -1;

        // printf("%04lx\n", c);

        switch (c) {
        case CTRL('L'):
            // reprint-line
            //
            reprint = true;
            break;

        case CTRL('P'):
        case KEY_UP:
            // previous-history
            //
            if (hline < 1)
                break;

            hline--;
            strncpy(s, hist_get(h, hline), n-1);

            len = strlen(s);
            cursor = len;
            reprint = true;
            break;

        case CTRL('N'):
        case KEY_DOWN:
            // next-history
            //

            // break if no history is available
            if (!h)
                break;

            if (hline < h->lines)
                hline++;

            if (hline == h->lines)
                s[0] = 0;
            else
                strncpy(s, hist_get(h, hline), n-1);

            len = strlen(s);
            cursor = len;
            reprint = true;
            break;

        case CTRL('R'):
        case KEY_PAGE_UP:
            // reverse-search-history
            //
            if (!h)
                break;

            match = 0;
            lastmatch = -1;
            for(;;) {
                match = hist_find(h, match, s, cursor);
                if (match == -1 || match >= hline)
                    break;
                lastmatch = match;
                match++;
            }

            if (lastmatch != -1) {
                strncpy(s, hist_get(h, lastmatch), n-1);
                hline   = lastmatch;
                len     = strlen(s);
                reprint = true;
            }
            break;

        case CTRL('S'):
        case KEY_PAGE_DOWN:
            // forward-search-history
            //
            if (!h)
                break;

            match = hist_find(h, hline+1, s, cursor);
            if (match != -1) {
                strncpy(s, hist_get(h, match), n-1);
                hline   = match;
                len     = strlen(s);
                reprint = true;
            }
            break;

        case '\t':
            // complete
            //
            cursor  = cmd_completion(s, cursor, n);
            len     = strlen(s);
            reprint = true;
            break;

        case CTRL('B'):
        case KEY_LEFT:
            // backward-char
            //
            if (cursor <= 0)
                break;
            printf(ANSI_LEFT);
            cursor--;
            break;

        case CTRL('F'):
        case KEY_RIGHT:
            // forward-char
            //
            if (cursor >= len)
                break;
            printf(ANSI_RIGHT);
            cursor++;
            break;

        case CTRL('A'):
        case KEY_HOME1:
        case KEY_HOME2:
        case KEY_HOME3:
            // beginning-of-line
            //
            if (cursor > 0)
                printf(ANSI_LEFTn(%d), cursor);
            cursor = 0;
            break;

        case CTRL('E'):
        case KEY_END1:
        case KEY_END2:
        case KEY_END3:
            // end-of-line
            //
            if (len-cursor > 0)
                printf(ANSI_RIGHTn(%d), len-cursor);
            cursor = len;
            break;

        case CTRL('K'):
            // kill-line
            //
            s[cursor] = 0;
            len = cursor;
            printf(ANSI_ERASE_EOL);
            break;

        case CTRL('U'):
            // kill-backwards
            //
            len -= cursor;
            memmove(s, &s[cursor], len+1);
            if (cursor > 0)
                printf(ANSI_LEFTn(%d) ANSI_DELn(%d), cursor, cursor);
            cursor = 0;
            break;

        case CTRL('D'):
        case KEY_DELETE:
            // delete-char
            //
            if (cursor >= len)
              break;
            printf(ANSI_DEL);

            memmove(&s[cursor], &s[cursor+1], len-cursor);
            len--;
            break;

        case CTRL('H'):
        case 0x7f:
            // backward-delete-char
            //
            if (cursor==0)
                break;
            printf(ANSI_LEFT ANSI_DEL);

            cursor--;
            memmove(&s[cursor], &s[cursor+1], len-cursor);
            len--;
            break;

        case '\n':
        case '\r':
            // accept-line
            //
            goto  done;
            break;

        case CTRL('C'):
            // reject-line
            //
            len = 0;
            cursor = 0;
            s[0] = 0;
            goto done;

        default:
            // insert character
            //
            if (len >= n-1 || c<32) {
                putchar('\a');
                break;
            }
            putchar(c);

            memmove(&s[cursor+1], &s[cursor], len-cursor+1);
            s[cursor++] = c;
            len++;
            break;
        }
    };

done:
    printf(ANSI_DISABLE_INSERT "\n");

    // append the line to the history, if it is
    // different from the most recent one.
    //
    if (h && len > 0 && strcmp(s, hist_get(h, h->lines-1)))
        hist_add(h, s);

    return len;
}

