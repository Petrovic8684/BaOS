#include <stdio.h>
#include "internal/syscalls.h"
#include <stdlib.h>
#include "stdio/tty_internal.h"

void read_line(char **buf_ptr)
{
    size_t bufsize = 64;
    size_t len = 0;
    size_t cursor = 0;
    char *buf = malloc(bufsize);
    if (!buf)
        return;

    int start_row = sys_get_cursor_row();
    int start_col = sys_get_cursor_col();

    while (1)
    {
        unsigned char c = sys_read();

        if (c == '\n' || c == '\r')
        {
            if (len + 1 > bufsize)
            {
                char *tmp = realloc(buf, len + 1);
                if (!tmp)
                {
                    free(buf);
                    return;
                }
                buf = tmp;
                bufsize = len + 1;
            }
            buf[len] = 0;
            ensure_space_and_scroll(&start_row, start_col, (int)len, (int)len);
            update_cursor(start_row, start_col + (int)len);
            write("\n");
            break;
        }

        if (c == 3)
        {
            if (cursor > 0)
                cursor--;
            update_cursor(start_row, start_col + cursor);
            continue;
        }
        if (c == 4)
        {
            if (cursor < len)
                cursor++;
            update_cursor(start_row, start_col + cursor);
            continue;
        }

        if (c == 8 && cursor > 0)
        {
            int old_len = (int)len;
            for (int j = (int)cursor - 1; j < (int)len - 1; j++)
                buf[j] = buf[j + 1];
            len--;
            cursor--;
            buf[len] = 0;
            ensure_space_and_scroll(&start_row, start_col, (int)len, old_len);
            redraw_buffer(buf, (int)len, start_row, start_col);
            if (old_len > (int)len)
            {
                update_cursor(start_row, start_col + (int)len);
                write("\x1B[J");
            }
            update_cursor(start_row, start_col + cursor);
            continue;
        }

        if (c == 127 && cursor < len)
        {
            int old_len = (int)len;
            for (int j = (int)cursor; j < (int)len - 1; j++)
                buf[j] = buf[j + 1];
            len--;
            buf[len] = 0;
            ensure_space_and_scroll(&start_row, start_col, (int)len, old_len);
            redraw_buffer(buf, (int)len, start_row, start_col);
            if (old_len > (int)len)
            {
                update_cursor(start_row, start_col + (int)len);
                write("\x1B[J");
            }
            update_cursor(start_row, start_col + cursor);
            continue;
        }

        if (c >= 128 && c <= 131)
        {
            int old_len = (int)len;
            if (c == 128 && cursor > 0)
            {
                int tail = (int)len - (int)cursor;
                for (int k = 0; k < tail; k++)
                    buf[k] = buf[k + cursor];
                len = tail;
                cursor = 0;
            }
            else if (c == 129 && cursor > 0)
            {
                int i = (int)cursor;
                while (i > 0 && buf[i - 1] == ' ')
                    i--;
                while (i > 0 && buf[i - 1] != ' ')
                    i--;
                int del_count = (int)cursor - i;
                if (del_count > 0)
                {
                    for (int k = i; k < (int)len - del_count; k++)
                        buf[k] = buf[k + del_count];
                    len -= del_count;
                    cursor = i;
                }
            }
            else if (c == 130 && cursor < len)
            {
                len = cursor;
            }
            else if (c == 131 && cursor < len)
            {
                int j = (int)cursor;
                while (j < (int)len && buf[j] == ' ')
                    j++;
                while (j < (int)len && buf[j] != ' ')
                    j++;
                int del_count = j - (int)cursor;
                if (del_count > 0)
                {
                    for (int k = (int)cursor; k < (int)len - del_count; k++)
                        buf[k] = buf[k + del_count];
                    len -= del_count;
                }
            }

            buf[len] = 0;
            ensure_space_and_scroll(&start_row, start_col, (int)len, old_len);
            redraw_buffer(buf, (int)len, start_row, start_col);
            if (old_len > (int)len)
            {
                update_cursor(start_row, start_col + (int)len);
                write("\x1B[J");
            }
            update_cursor(start_row, start_col + cursor);
            continue;
        }

        if (c < 32 || c > 126)
            continue;

        if (len + 1 >= bufsize)
        {
            bufsize *= 2;
            char *tmp = realloc(buf, bufsize);
            if (!tmp)
            {
                free(buf);
                return;
            }
            buf = tmp;
        }

        int old_len = (int)len;
        for (int j = (int)len; j > (int)cursor; j--)
            buf[j] = buf[j - 1];
        buf[cursor] = c;
        len++;
        cursor++;
        buf[len] = 0;

        ensure_space_and_scroll(&start_row, start_col, (int)len, old_len);
        redraw_buffer(buf, (int)len, start_row, start_col);
        update_cursor(start_row, start_col + cursor);
    }

    *buf_ptr = buf;
}
