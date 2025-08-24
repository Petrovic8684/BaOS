#include "filling.h"

#define MAX_LINES 25
#define MAX_COLS 80
#define WELCOME_LINES 2

char buffer[MAX_LINES][MAX_COLS];
int cur_row = 0, cur_col = 0;

void editor_redraw(int welcome_active)
{
    int offset = welcome_active ? WELCOME_LINES : 0;

    clear();

    if (welcome_active)
    {
        write_colored("FILLING editor: Press F5 to exit and save or ESC to exit without saving.\n", 0x0E);
        write("\n");
    }

    for (int r = 0; r < MAX_LINES; r++)
    {
        for (int c = 0; c < MAX_COLS; c++)
        {
            char ch = buffer[r][c] ? buffer[r][c] : ' ';
            draw_char_at(r + offset, c, ch);
        }
    }

    row = cur_row + offset;
    col = cur_col;
    update_cursor();
}

void editor_load_file(const char *filename)
{
    unsigned char file_buf[MAX_LINES * MAX_COLS];
    unsigned int file_size = 0;

    int res = fs_read_file_buffer(filename, file_buf, sizeof(file_buf), &file_size);
    if (res != FS_OK || file_size == 0)
        return;

    unsigned int row = 0, col = 0;

    for (unsigned int i = 0; i < file_size; i++)
    {
        unsigned char c = file_buf[i];

        if (c == '\n')
        {
            buffer[row][col] = 0;
            row++;
            col = 0;

            if (row >= MAX_LINES)
                break;
        }
        else
        {
            if (col < MAX_COLS - 1)
            {
                buffer[row][col++] = c;
                buffer[row][col] = 0;
            }
        }
    }

    cur_row = row < MAX_LINES ? row : MAX_LINES - 1;
    cur_col = col;
}

void editor_save_file(const char *filename)
{
    char text[MAX_LINES * MAX_COLS];
    int idx = 0;

    for (int r = 0; r < MAX_LINES; r++)
    {
        int line_len = str_count(buffer[r]);
        for (int c = 0; c < line_len; c++)
            text[idx++] = buffer[r][c];

        if (r < MAX_LINES - 1 && (line_len > 0 || buffer[r + 1][0] != 0))
            text[idx++] = '\n';
    }

    text[idx] = 0;

    if (idx > 0)
        fs_write_file(filename, text);
}

void filling_main(const char *filename)
{
    cur_row = 0;
    cur_col = 0;
    mem_set(buffer, 0, sizeof(buffer));

    int welcome_active = 1;

    editor_load_file(filename);

    int last_row = 0;
    for (int r = 0; r < MAX_LINES; r++)
        if (str_count(buffer[r]) > 0)
            last_row = r;

    cur_row = last_row;
    cur_col = str_count(buffer[cur_row]);

    editor_redraw(welcome_active);

    while (1)
    {
        char c = read();

        if (welcome_active && (c >= 32 && c <= 126 || c == '\n' || c == '\b' ||
                               c == ARR_UP || c == ARR_DOWN || c == ARR_LEFT || c == ARR_RIGHT))
        {
            welcome_active = 0;
            editor_redraw(welcome_active);
        }

        if (c == 26)
        {
            editor_save_file(filename);
            return;
        }

        if (c == 27)
        {
            return;
        }

        if (c == '\b')
        {
            if (cur_col > 0)
            {
                for (int i = cur_col - 1; i < str_count(buffer[cur_row]); i++)
                    buffer[cur_row][i] = buffer[cur_row][i + 1];
                cur_col--;
            }
            else if (cur_row > 0)
            {
                cur_row--;
                cur_col = str_count(buffer[cur_row]);
            }
        }
        else if (c == '\n')
        {
            if (cur_row < MAX_LINES - 1)
            {
                for (int r = MAX_LINES - 1; r > cur_row; r--)
                    str_copy_fixed(buffer[r], buffer[r - 1], MAX_COLS);

                char temp[MAX_COLS];
                int right_len = str_count(buffer[cur_row]) - cur_col;
                for (int i = 0; i < right_len; i++)
                    temp[i] = buffer[cur_row][cur_col + i];
                temp[right_len] = 0;

                buffer[cur_row][cur_col] = 0;
                str_copy_fixed(buffer[cur_row + 1], temp, MAX_COLS);

                cur_row++;
                cur_col = 0;
            }
        }
        else if (c >= 32 && c <= 126)
        {
            int len = str_count(buffer[cur_row]);
            if (len < MAX_COLS - 1)
            {
                for (int i = len; i > cur_col; i--)
                    buffer[cur_row][i] = buffer[cur_row][i - 1];
                buffer[cur_row][cur_col++] = c;
                buffer[cur_row][len + 1] = 0;
            }
        }
        else if (c == ARR_UP)
        {
            if (cur_row > 0)
                cur_row--;
            if (cur_col > str_count(buffer[cur_row]))
                cur_col = str_count(buffer[cur_row]);
        }
        else if (c == ARR_DOWN)
        {
            if (cur_row < MAX_LINES - 1)
                cur_row++;
            if (cur_col > str_count(buffer[cur_row]))
                cur_col = str_count(buffer[cur_row]);
        }
        else if (c == ARR_LEFT)
        {
            if (cur_col > 0)
                cur_col--;
            else if (cur_row > 0)
            {
                cur_row--;
                cur_col = str_count(buffer[cur_row]);
            }
        }
        else if (c == ARR_RIGHT)
        {
            if (cur_col < str_count(buffer[cur_row]))
                cur_col++;
        }

        editor_redraw(welcome_active);
    }
}
