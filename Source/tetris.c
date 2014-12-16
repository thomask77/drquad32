#include <stdio.h>
#include <stdlib.h>
#include "tetris.h"
#include "ustime.h"
#include "ansi.h"
#include "syscalls.h"

struct tetris_level {
    int score;
    int usec;
};

struct tetris_block {
    char data[5][5];
    int w;
    int h;
};

struct tetris {
    char **game;
    int w, h;
    int level;
    int gameover;
    int score;
    struct tetris_block current;
    int x, y;
};

struct tetris_block blocks[] = {
    { { "##", "##" },       2, 2 },
    { { " X ", "XXX" },     3, 2 },
    { { "@@@@" },           4, 1 },
    { { "OO", "O ", "O " }, 2, 3 },
    { { "&&", " &", " &" }, 2, 3 },
    { { "ZZ ", " ZZ" },     3, 2 },
    { { " SS", "SS " },     3, 2 }
};

struct tetris_level levels[] = {
    {      0,   1200 },
    {   1500,    900 },
    {   8000,    700 },
    {  20000,    500 },
    {  40000,    400 },
    {  75000,    300 },
    { 100000,    200 }
};

#define TETRIS_PIECES (sizeof(blocks)/sizeof(struct tetris_block))
#define TETRIS_LEVELS (sizeof(levels)/sizeof(struct tetris_level))

static void tetris_init(struct tetris *t, int w, int h)
{
    t->level = 1;
    t->score = 0;
    t->gameover = 0;
    t->w = w;
    t->h = h;

    // TODO: one big malloc
    t->game = malloc(sizeof(char*) * w);

    for (int x = 0; x < w; x++) {
        t->game[x] = malloc(sizeof(char) * h);
        for (int y = 0; y < h; y++)
            t->game[x][y] = ' ';
    }
}

void tetris_clean(struct tetris *t)
{
    for (int x = 0; x < t->w; x++)
        free(t->game[x]);

    free(t->game);
}

void tetris_print(struct tetris *t)
{
    printf(ANSI_HOME);

    for (int x = 0; x < 2; x++)
        printf("\n");

    printf("[LEVEL: %d | SCORE: %d]\n", t->level, t->score);

    for (int x = 0; x < 2 * t->w + 2; x++)
        printf("~");

    printf("\n");

    for (int y = 0; y < t->h; y++) {
        printf("!");
        for (int x = 0; x < t->w; x++) {
            if (x >= t->x && y >= t->y && x < (t->x + t->current.w) && y < (t->y + t->current.h)
                    && t->current.data[y - t->y][x - t->x] != ' ')
                printf("%c ", t->current.data[y - t->y][x - t->x]);
            else
                printf("%c ", t->game[x][y]);
        }
        printf("!\n");
    }

    for (int x = 0; x < 2 * t->w + 2; x++)
        printf("~");

    printf("\n");
}

int tetris_hittest(struct tetris *t)
{
    struct tetris_block b = t->current;

    for (int x = 0; x < b.w; x++) {
        for (int y = 0; y < b.h; y++) {
            int X = t->x + x;
            int Y = t->y + y;
            if (X < 0 || X >= t->w)
                return 1;
            if (b.data[y][x] != ' ') {
                if ((Y >= t->h) || (X >= 0 && X < t->w && Y >= 0 && t->game[X][Y] != ' ')) {
                    return 1;
                }
            }
        }
    }
    return 0;
}


void tetris_new_block(struct tetris *t)
{
    t->current = blocks[rand() % TETRIS_PIECES];
    t->x = (t->w / 2) - (t->current.w / 2);
    t->y = 0;

    if (tetris_hittest(t))
        t->gameover = 1;
}


void tetris_print_block(struct tetris *t)
{
    struct tetris_block b = t->current;

    for (int x = 0; x < b.w; x++) {
        for (int y = 0; y < b.h; y++) {
            if (b.data[y][x] != ' ')
                t->game[t->x + x][t->y + y] = b.data[y][x];
        }
    }
}

void tetris_rotate(struct tetris *t)
{
    struct tetris_block b = t->current;
    struct tetris_block s = b;

    b.w = s.h;
    b.h = s.w;

    for (int x = 0; x < s.w; x++) {
        for (int y = 0; y < s.h; y++) {
            b.data[x][y] = s.data[s.h - y - 1][x];
        }
    }

    int x = t->x;
    int y = t->y;
    t->x -= (b.w - s.w) / 2;
    t->y -= (b.h - s.h) / 2;
    t->current = b;

    if (tetris_hittest(t)) {
        t->current = s;
        t->x = x;
        t->y = y;
    }
}

void tetris_gravity(struct tetris *t)
{
    t->y++;
    if (tetris_hittest(t)) {
        t->y--;
        tetris_print_block(t);
        tetris_new_block(t);
    }
}

void tetris_fall(struct tetris *t, int l)
{
    for (int y = l; y > 0; y--) {
        for (int x = 0; x < t->w; x++)
            t->game[x][y] = t->game[x][y - 1];
    }
    for (int x = 0; x < t->w; x++)
        t->game[x][0] = ' ';
}

void tetris_check_lines(struct tetris *t)
{
    int l;
    int p = 100;

    for (int y = t->h - 1; y >= 0; y--) {
        l = 1;
        for (int x = 0; x < t->w && l; x++) {
            if (t->game[x][y] == ' ') {
                l = 0;
            }
        }
        if (l) {
            t->score += p;
            p *= 2;
            tetris_fall(t, y);
            y++;
        }
    }
}

int tetris_level(struct tetris *t)
{
    for (int i = 0; i < TETRIS_LEVELS; i++) {
        if (t->score >= levels[i].score)
            t->level = i + 1;
        else
            break;
    }

    return levels[t->level - 1].usec;
}

void tetris_run(int w, int h)
{
    srand(get_us_time64());

    struct tetris t;
    tetris_init(&t, w, h);
    tetris_new_block(&t);

    int count = 0;
    int tm = 1000;

    printf(ANSI_CLEAR ANSI_CURSOR_OFF);

    while (!t.gameover) {
        delay_us(tm);
        count++;
        if (count % 50 == 0) {
            tetris_print(&t);
        }
        if (count % 350 == 0) {
            tetris_gravity(&t);
            tetris_check_lines(&t);
        }

        while (stdin_chars_avail()) {
            switch (getchar()) {
            case 'a':
                t.x--;
                if (tetris_hittest(&t))
                    t.x++;
                break;
            case 'd':
                t.x++;
                if (tetris_hittest(&t))
                    t.x--;
                break;
            case 's':
                tetris_gravity(&t);
                break;
            case ' ':
                tetris_rotate(&t);
                break;
            case 3: /*CTRL-C*/
                goto cleanup;
            }
        }
        tm = tetris_level(&t);
    }

    tetris_print(&t);
    printf("*** GAME OVER ***\n");

cleanup:
    tetris_clean(&t);
    printf(ANSI_CURSOR_ON);
}

// -------------------- Shell commands --------------------
//
#include "command.h"

static void cmd_tetris(void)
{
    tetris_run(10, 20);
}

SHELL_CMD(tetris, (cmdfunc_t)cmd_tetris, "Tetris!")
