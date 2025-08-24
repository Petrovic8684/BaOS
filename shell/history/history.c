#include "history.h"

static char history[HISTORY_SIZE][MAX_CMD_LEN];
static int history_count = 0;
static int history_index = 0;

void history_init(void)
{
    history_count = 0;
    history_index = 0;
}

void history_add(const char *cmd)
{
    if (!cmd[0])
        return;

    if (history_count < HISTORY_SIZE)
    {
        str_copy_fixed(history[history_count], cmd, MAX_CMD_LEN);
        history_count++;
    }
    else
    {
        for (int i = 1; i < HISTORY_SIZE; i++)
            str_copy_fixed(history[i - 1], history[i], MAX_CMD_LEN);

        str_copy_fixed(history[HISTORY_SIZE - 1], cmd, MAX_CMD_LEN);
    }

    history_index = history_count;
}

const char *history_prev(void)
{
    if (history_count == 0 || history_index == 0)
        return 0;

    history_index--;
    return history[history_index];
}

const char *history_next(void)
{
    if (history_count == 0 || history_index >= history_count)
        return 0;

    history_index++;
    if (history_index == history_count)
        return 0;

    return history[history_index];
}

void history_reset_index(void)
{
    history_index = history_count;
}

void history_read(char *buffer, int max_len, int prompt_len)
{
    int buf_idx = 0;
    int caret_pos = 0;
    int prev_len = 0;
    buffer[0] = '\0';

    col = prompt_len;
    update_cursor();
    history_reset_index();

    while (1)
    {
        char input = read();

        if (input == (char)ARR_UP)
        {
            const char *prev = history_prev();
            if (prev)
            {
                str_copy_fixed(buffer, prev, max_len);
                buf_idx = str_count(buffer);
                caret_pos = buf_idx;

                redraw_and_clear(buffer, buf_idx, prompt_len, prev_len);
                prev_len = buf_idx;

                col = prompt_len + caret_pos;
                update_cursor();
            }
            continue;
        }

        if (input == (char)ARR_DOWN)
        {
            const char *next = history_next();
            if (next)
            {
                str_copy_fixed(buffer, next, max_len);
                buf_idx = str_count(buffer);
                caret_pos = buf_idx;

                redraw_and_clear(buffer, buf_idx, prompt_len, prev_len);
                prev_len = buf_idx;

                col = prompt_len + caret_pos;
                update_cursor();
            }
            else
            {
                buf_idx = caret_pos = 0;
                buffer[0] = '\0';

                redraw_and_clear(buffer, 0, prompt_len, prev_len);
                prev_len = 0;

                col = prompt_len;
                update_cursor();
            }
            continue;
        }

        if (input == (char)ARR_LEFT)
        {
            if (caret_pos > 0)
            {
                caret_pos--;
                col--;
                update_cursor();
            }
            continue;
        }

        if (input == (char)ARR_RIGHT)
        {
            if (caret_pos < buf_idx)
            {
                caret_pos++;
                col++;
                update_cursor();
            }
            continue;
        }

        if (input == '\n')
        {
            buffer[buf_idx] = '\0';
            row++;
            col = 0;
            scroll();
            update_cursor();

            history_add(buffer);
            break;
        }

        if (input == '\b')
        {
            if (caret_pos > 0)
            {
                for (int i = caret_pos - 1; i < buf_idx - 1; i++)
                    buffer[i] = buffer[i + 1];
                buf_idx--;
                caret_pos--;

                redraw_and_clear(buffer, buf_idx, prompt_len, prev_len);
                prev_len = buf_idx;

                col = prompt_len + caret_pos;
                update_cursor();
            }

            continue;
        }

        if (buf_idx < max_len - 1)
        {
            for (int i = buf_idx; i > caret_pos; i--)
                buffer[i] = buffer[i - 1];

            buffer[caret_pos] = input;
            buf_idx++;
            caret_pos++;

            redraw_and_clear(buffer, buf_idx, prompt_len, prev_len);
            prev_len = buf_idx;

            col = prompt_len + caret_pos;
            update_cursor();
        }
    }

    history_reset_index();
}
