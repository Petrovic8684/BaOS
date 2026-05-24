#include "stdio/tty_internal.h"
#include "internal/syscalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TTY_ROWS 25
#define TTY_COLS 80
#define TTY_LAST_ROW (TTY_ROWS - 1)
#define TTY_LAST_COL (TTY_COLS - 1)

enum ansi_state
{
    ANSI_NORMAL_ST = 0,
    ANSI_ESC_ST,
    ANSI_CSI_ST
};

static int tty_row = 0;
static int tty_col = 0;
static unsigned char tty_attr = 0x07;
static int ansi_state = ANSI_NORMAL_ST;
static char ansi_params[64];
static int ansi_params_len = 0;

void send_move_syscall(int r, int c)
{
    char seq[32];
    int n = snprintf(seq, sizeof(seq), "\x1B[%u;%uH", (unsigned int)r, (unsigned int)c);
    if (n > 0 && n < (int)sizeof(seq))
        write(seq);
}


void tty_set_position(int row, int col)
{
    clamp_cursor(&row, &col);
    tty_row = row;
    tty_col = col;
}


void clamp_cursor(int *row, int *col)
{
    if (*row < 0)
        *row = 0;
    else if (*row > TTY_LAST_ROW)
        *row = TTY_LAST_ROW;

    if (*col < 0)
        *col = 0;
    else if (*col > TTY_LAST_COL)
        *col = TTY_LAST_COL;
}


void scroll_up(int n)
{
    if (n <= 0)
        return;

    char seq[16];
    int len = snprintf(seq, sizeof(seq), "\x1B[%uS", (unsigned int)n);
    if (len > 0 && len < (int)sizeof(seq))
        write(seq);

    int r = sys_get_cursor_row();
    int c = sys_get_cursor_col();

    clamp_cursor(&r, &c);

    tty_row = r;
    tty_col = c;

    send_move_syscall(tty_row + 1, tty_col + 1);
}


void tty_wrap_char(unsigned char ch)
{
    switch (ch)
    {
    case '\n':
        tty_col = 0;
        if (tty_row < TTY_LAST_ROW)
            tty_row++;
        return;

    case '\r':
        tty_col = 0;
        return;

    default:
        if (++tty_col >= TTY_COLS)
        {
            tty_col = 0;
            if (tty_row < TTY_LAST_ROW)
                tty_row++;
        }
        return;
    }
}


void tty_parse_after_write(const char *s)
{
    for (const unsigned char *p = (const unsigned char *)s; *p; p++)
    {
        unsigned char ch = *p;
        switch (ansi_state)
        {
        case ANSI_NORMAL_ST:
            if (ch == 0x1B)
            {
                ansi_state = ANSI_ESC_ST;
                ansi_params_len = 0;
                ansi_params[0] = '\0';
            }
            else if (ch == '\n')
            {
                tty_col = 0;
                tty_row++;
                if (tty_row > TTY_LAST_ROW)
                    tty_row = TTY_LAST_ROW;
            }
            else if (ch == '\r')
            {
                tty_col = 0;
            }
            else if (ch == '\t')
            {
                int spaces = 4 - (tty_col % 4);
                for (int i = 0; i < spaces; i++)
                    tty_wrap_char(' ');
            }
            else if (ch == '\b')
            {
                if (tty_col > 0)
                    tty_col--;
                else if (tty_row > 0)
                {
                    tty_row--;
                    tty_col = TTY_LAST_COL;
                }
            }
            else
            {
                tty_wrap_char(ch);
            }
            break;

        case ANSI_ESC_ST:
            if (ch == '[')
            {
                ansi_state = ANSI_CSI_ST;
                ansi_params_len = 0;
                ansi_params[0] = '\0';
            }
            else
            {
                ansi_state = ANSI_NORMAL_ST;
            }
            break;

        case ANSI_CSI_ST:
            if (ch >= 0x40 && ch <= 0x7E)
            {
                ansi_params[ansi_params_len] = '\0';
                int pvals[8];
                for (int i = 0; i < 8; i++)
                    pvals[i] = -1;
                int pcount = 0;

                const char *sp = ansi_params;
                while (*sp && pcount < 8)
                {
                    while (*sp == ';')
                        sp++;
                    if (!*sp)
                        break;
                    int val = 0;
                    int seen = 0;
                    while (*sp >= '0' && *sp <= '9')
                    {
                        seen = 1;
                        val = val * 10 + (*sp - '0');
                        sp++;
                    }
                    pvals[pcount++] = seen ? val : -1;
                    if (*sp == ';')
                        sp++;
                }

                char final = (char)ch;
                switch (final)
                {
                case 'A':
                {
                    int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    tty_row -= n;
                    if (tty_row < 0)
                        tty_row = 0;
                    break;
                }
                case 'B':
                {
                    int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    tty_row += n;
                    if (tty_row > TTY_LAST_ROW)
                        tty_row = TTY_LAST_ROW;
                    break;
                }
                case 'C':
                {
                    int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    tty_col += n;
                    if (tty_col > TTY_LAST_COL)
                        tty_col = TTY_LAST_COL;
                    break;
                }
                case 'D':
                {
                    int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    tty_col -= n;
                    if (tty_col < 0)
                        tty_col = 0;
                    break;
                }
                case 'H':
                case 'f':
                {
                    int r = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    int c = (pcount >= 2 && pvals[1] > 0) ? pvals[1] : 1;
                    tty_row = r - 1;
                    tty_col = c - 1;
                    clamp_cursor(&tty_row, &tty_col);
                    break;
                }
                case 'J':
                {
                    int mode = (pcount >= 1 && pvals[0] >= 0) ? pvals[0] : 0;
                    if (mode == 2)
                    {
                        tty_row = 0;
                        tty_col = 0;
                    }
                    break;
                }
                case 'K':
                    break;
                case 'm':
                {
                    if (pcount == 0)
                        tty_attr = 0x07;
                    else
                    {
                        for (int i = 0; i < pcount; i++)
                        {
                            int p = pvals[i] == -1 ? 0 : pvals[i];
                            if (p == 0)
                                tty_attr = 0x07;
                            else if (p == 1)
                                tty_attr |= 0x08;
                            else if (p >= 30 && p <= 37)
                            {
                                int fg = (p - 30) & 0x07;
                                unsigned char bg = tty_attr & 0xF0;
                                unsigned char intensity = tty_attr & 0x08;
                                tty_attr = bg | ((fg | intensity) & 0x0F);
                            }
                            else if (p >= 40 && p <= 47)
                            {
                                int bg = (p - 40) & 0x07;
                                unsigned char fg = tty_attr & 0x0F;
                                tty_attr = (unsigned char)((bg << 4) | fg);
                            }
                        }
                    }
                    break;
                }
                }

                ansi_state = ANSI_NORMAL_ST;
                ansi_params_len = 0;
            }
            else
            {
                if (ansi_params_len < (int)sizeof(ansi_params) - 1)
                    ansi_params[ansi_params_len++] = (char)ch;
                else
                {
                    ansi_state = ANSI_NORMAL_ST;
                    ansi_params_len = 0;
                }
            }
            break;
        }
    }
}


void update_cursor(int new_row, int new_col)
{
    if (new_col < 0)
        new_col = 0;

    if (new_col >= TTY_COLS)
    {
        new_row += new_col / TTY_COLS;
        new_col = new_col % TTY_COLS;
    }

    clamp_cursor(&new_row, &new_col);

    tty_row = new_row;
    tty_col = new_col;
    send_move_syscall(tty_row + 1, tty_col + 1);
}


void redraw_buffer(const char *buf, int len, int start_row, int start_col)
{
    send_move_syscall(start_row + 1, start_col + 1);

    if (len > 0)
    {
        char *tmp = (char *)malloc(len + 1);
        if (!tmp)
            return;
        memcpy(tmp, buf, len);
        tmp[len] = '\0';

        write(tmp);
        free(tmp);
    }

    int after_row = sys_get_cursor_row();
    int after_col = sys_get_cursor_col();
    clamp_cursor(&after_row, &after_col);

    send_move_syscall(after_row + 1, after_col + 1);

    write("\x1B[K");

    tty_row = after_row;
    tty_col = after_col;
}


void ensure_space_and_scroll(int *start_row, int start_col, int cur_len, int old_len)
{
    int old_rows = (start_col + old_len) / TTY_COLS;
    int new_rows = (start_col + cur_len) / TTY_COLS;
    if (new_rows <= old_rows)
        return;
    int rows_used = new_rows;
    int bottom_needed = *start_row + rows_used;
    if (bottom_needed > TTY_LAST_ROW)
    {
        int scroll_lines = bottom_needed - TTY_LAST_ROW;
        scroll_up(scroll_lines);
        *start_row -= scroll_lines;
        if (*start_row < 0)
            *start_row = 0;
    }
}
