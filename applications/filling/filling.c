#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_COLS 80
#define WELCOME_LINES 2
#define TERM_ROWS 25
#define MAX_PRINTABLE (MAX_COLS)
#define MAX_FILE_NAME 16

static char **buffer = NULL;
static size_t num_lines = 0;
static size_t *line_sizes = NULL;
static size_t default_line_size = 128;

static char *clipboard = NULL;
static size_t clipboard_size = 0;

static char filename[128] = "NO NAME";
static int cur_row = 0, cur_col = 0;
static int modified = 0;
static int welcome_active = 1;
static int readonly_mode = 0;
static char status_msg[MAX_COLS] = "";
static int screen_row_offset = 0;
static int syntax_highlight = 0;
static int selecting = 0;
static int sel_start_row = 0, sel_start_col = 0;
static int sel_end_row = 0, sel_end_col = 0;

void editor_init(void)
{
    num_lines = 16;
    default_line_size = 128;

    buffer = malloc(num_lines * sizeof(char *));
    if (!buffer)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }

    line_sizes = malloc(num_lines * sizeof(size_t));
    if (!line_sizes)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }

    for (size_t i = 0; i < num_lines; i++)
    {
        line_sizes[i] = default_line_size;
        buffer[i] = calloc(line_sizes[i], 1);
        if (!buffer[i])
        {
            printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
            exit(1);
        }
        buffer[i][0] = '\0';
    }

    clipboard_size = 1024;
    clipboard = calloc(clipboard_size, 1);
    if (!clipboard)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }
    clipboard[0] = '\0';
}

void ensure_buffer_rows(size_t row);

void ensure_line_capacity(int row, size_t needed)
{
    if (row < 0)
        return;
    ensure_buffer_rows((size_t)row);
    if (needed <= line_sizes[row])
        return;

    size_t new_size = line_sizes[row] ? line_sizes[row] * 2 : default_line_size;
    while (new_size <= needed)
        new_size *= 2;

    char *p = realloc(buffer[row], new_size);
    if (!p)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }
    if (new_size > line_sizes[row])
        memset(p + line_sizes[row], 0, new_size - line_sizes[row]);

    buffer[row] = p;
    line_sizes[row] = new_size;
}

void ensure_buffer_rows(size_t row)
{
    if (row < num_lines)
        return;

    size_t new_num = num_lines ? num_lines : 1;
    while (new_num <= row)
        new_num *= 2;

    char **nbuf = realloc(buffer, new_num * sizeof(char *));
    size_t *nsz = realloc(line_sizes, new_num * sizeof(size_t));
    if (!nbuf || !nsz)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }
    buffer = nbuf;
    line_sizes = nsz;

    for (size_t i = num_lines; i < new_num; ++i)
    {
        line_sizes[i] = default_line_size;
        buffer[i] = calloc(line_sizes[i], 1);
        if (!buffer[i])
        {
            printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
            exit(1);
        }
        buffer[i][0] = '\0';
    }

    num_lines = new_num;
}

void clipboard_reset(void)
{
    if (!clipboard)
        return;
    clipboard[0] = '\0';
}

void clipboard_append_text(const char *text, size_t tlen)
{
    if (!clipboard)
        editor_init();

    size_t curr_len = strlen(clipboard);
    if (curr_len + tlen + 1 >= clipboard_size)
    {
        size_t new_size = clipboard_size ? clipboard_size * 2 : 1024;
        while (new_size <= curr_len + tlen)
            new_size *= 2;
        char *p = realloc(clipboard, new_size);
        if (!p)
        {
            printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
            exit(1);
        }
        clipboard = p;
        clipboard_size = new_size;
    }

    memcpy(clipboard + curr_len, text, tlen);
    clipboard[curr_len + tlen] = '\0';
}

static void reset_selection(void)
{
    selecting = 0;
    sel_start_row = sel_start_col = sel_end_row = sel_end_col = 0;
}

static void update_selection(int new_row, int new_col)
{
    if (!selecting)
    {
        sel_start_row = cur_row;
        sel_start_col = cur_col;
        selecting = 1;
    }
    sel_end_row = new_row;
    sel_end_col = new_col;
}

static int is_selected(int row, int col)
{
    if (!selecting)
        return 0;

    int start_row = sel_start_row, start_col = sel_start_col;
    int end_row = sel_end_row, end_col = sel_end_col;

    if (start_row > end_row || (start_row == end_row && start_col > end_col))
    {
        int tmp_row = start_row, tmp_col = start_col;
        start_row = end_row;
        start_col = end_col;
        end_row = tmp_row;
        end_col = tmp_col;
    }

    if (row < start_row || row > end_row)
        return 0;

    if (row == start_row && row == end_row)
        return col >= start_col && col < end_col;
    else if (row == start_row)
        return col >= start_col;
    else if (row == end_row)
        return col < end_col;
    else
        return 1;
}

static int visible_content_rows(void)
{
    int offset = welcome_active ? WELCOME_LINES : 0;
    int visible = TERM_ROWS - offset - 1;
    if (visible < 1)
        visible = 1;
    return visible;
}

static int content_rows_used_by_line(int logical_row)
{
    if (logical_row < 0 || (size_t)logical_row >= num_lines)
        return 1;
    int len = (int)strlen(buffer[logical_row]);
    if (len == 0)
        return 1;
    return (len + MAX_PRINTABLE - 1) / MAX_PRINTABLE;
}

static int total_content_rows(void)
{
    int total = 0;
    for (size_t i = 0; i < num_lines; ++i)
        total += content_rows_used_by_line((int)i);
    return total;
}

static int content_rows_before_logical(int logical_row)
{
    int total = 0;
    for (int i = 0; i < logical_row && (size_t)i < num_lines; ++i)
        total += content_rows_used_by_line(i);
    return total;
}

static int get_last_filled_logical_row(void)
{
    if (num_lines == 0)
        return 0;
    for (long i = (long)num_lines - 1; i >= 0; --i)
    {
        if (buffer[i][0] != '\0')
            return (int)i;
    }
    return 0;
}

static void ensure_cursor_visible(void)
{
    int offset = welcome_active ? WELCOME_LINES : 0;
    int visible = visible_content_rows();
    int before = content_rows_before_logical(cur_row);
    int in_row_index = cur_col / MAX_PRINTABLE;
    int cursor_content_index = before + in_row_index;
    int total = total_content_rows();
    if (total <= visible)
    {
        screen_row_offset = 0;
        return;
    }
    if (cursor_content_index < 0)
        cursor_content_index = 0;
    if (cursor_content_index >= total)
        cursor_content_index = total - 1;
    if (cursor_content_index < screen_row_offset)
    {
        screen_row_offset = cursor_content_index;
    }
    else if (cursor_content_index >= screen_row_offset + visible)
    {
        screen_row_offset = cursor_content_index - visible + 1;
    }
    if (screen_row_offset < 0)
        screen_row_offset = 0;
    int max_offset = total - visible;
    if (max_offset < 0)
        max_offset = 0;
    if (screen_row_offset > max_offset)
        screen_row_offset = max_offset;
}

static void print_colored_c(int row_index, const char *line)
{
    const char *keywords[] = {
        "int", "char", "float", "double", "void", "if", "else", "for", "while",
        "return", "struct", "typedef", "enum", "const", "static", "break", "continue",
        "switch", "case", "default", "sizeof", "do", "goto"};

    int in_string = 0;
    int in_char = 0;
    int in_comment = 0;

    for (int i = 0; line[i]; i++)
    {
        char c = line[i];

        int selected = is_selected(row_index, i);

        if (in_comment)
        {
            if (selected)
                printf("\033[1;37;46m%c\033[0m", c);
            else
                putchar(c);

            if (c == '\n')
                in_comment = 0;
        }
        else if (in_string)
        {
            if (selected)
                printf("\033[1;37;46m%c\033[0m", c);
            else
                printf("\033[1;32m%c\033[0m", c);

            if (c == '"' && line[i - 1] != '\\')
                in_string = 0;
        }
        else if (in_char)
        {
            if (selected)
                printf("\033[1;37;46m%c\033[0m", c);
            else
                printf("\033[1;32m%c\033[0m", c);

            if (c == '\'' && line[i - 1] != '\\')
                in_char = 0;
        }
        else
        {
            if (c == '/' && line[i + 1] == '/')
            {
                for (int j = i; line[j]; j++)
                {
                    selected = is_selected(row_index, j);
                    if (selected)
                        printf("\033[1;37;46m%c\033[0m", line[j]);
                    else
                        printf("\033[32m%c\033[0m", line[j]);
                }
                break;
            }
            else if (c == '"')
            {
                in_string = 1;
                if (selected)
                    printf("\033[1;37;46m%c\033[0m", c);
                else
                    printf("\033[1;32m%c\033[0m", c);
            }
            else if (c == '\'')
            {
                in_char = 1;
                if (selected)
                    printf("\033[1;37;46m%c\033[0m", c);
                else
                    printf("\033[1;32m%c\033[0m", c);
            }
            else if (isdigit(c))
            {
                int start = i;
                while (isdigit(line[i]) || line[i] == '.' || line[i] == 'x' || (line[i] >= 'a' && line[i] <= 'f') || (line[i] >= 'A' && line[i] <= 'F'))
                    i++;
                int len = i - start;
                for (int k = 0; k < len; k++)
                {
                    selected = is_selected(row_index, start + k);
                    if (selected)
                        printf("\033[1;37;46m%c\033[0m", line[start + k]);
                    else
                        printf("\033[1;35m%c\033[0m", line[start + k]);
                }
                i--;
            }
            else if (isalpha(c) || c == '_')
            {
                int start = i;
                while (isalnum(line[i]) || line[i] == '_')
                    i++;
                int len = i - start;
                char word[64];
                if (len < 64)
                {
                    strncpy(word, line + start, len);
                    word[len] = '\0';

                    int is_kw = 0;
                    for (int k = 0; k < (int)(sizeof(keywords) / sizeof(keywords[0])); k++)
                        if (strcmp(word, keywords[k]) == 0)
                        {
                            is_kw = 1;
                            break;
                        }

                    for (int k = 0; k < len; k++)
                    {
                        selected = is_selected(row_index, start + k);
                        if (selected)
                            printf("\033[1;37;46m%c\033[0m", word[k]);
                        else if (is_kw)
                        {
                            if (strcmp(word, "const") == 0 || strcmp(word, "static") == 0)
                                printf("\033[33m%c\033[0m", word[k]);
                            else if (strcmp(word, "sizeof") == 0)
                                printf("\033[1;32m%c\033[0m", word[k]);
                            else
                                printf("\033[31m%c\033[0m", word[k]);
                        }
                        else
                        {
                            int j = start + len;
                            while (line[j] == ' ' || line[j] == '\t')
                                j++;
                            if (line[j] == '(')
                                printf("\033[1;33m%c\033[0m", word[k]);
                            else
                                printf("\033[1;34m%c\033[0m", word[k]);
                        }
                    }
                }
                i--;
            }
            else
            {
                if (selected)
                    printf("\033[1;37;46m%c\033[0m", c);
                else
                    putchar(c);
            }
        }
    }
}

static void clear_screen_and_home(void)
{
    printf("\033[2J");
    printf("\033[H");
}

static void set_status(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_msg, sizeof(status_msg), fmt, ap);
    status_msg[sizeof(status_msg) - 1] = '\0';
    va_end(ap);
}

static int will_wrap_to_last_row(int row, int col)
{
    (void)row;
    (void)col;
    return 0;
}

static void clear_status(void) { status_msg[0] = '\0'; }

static void move_cursor_to_status_input(void)
{
    int col = (int)strlen(status_msg) + 2;
    if (col < 2)
        col = 2;
    if (col > MAX_COLS)
        col = MAX_COLS;
    printf("\033[%d;%dH", TERM_ROWS, col);
    fflush(stdout);
}

static void editor_redraw(void)
{
    printf("\033[2J");
    printf("\033[H");
    int line_no = 1;
    if (welcome_active)
    {
        printf("\033[%d;1H", line_no++);
        printf("\033[30;47m BaOS FILLING text editor: Press ESC to exit. \033[0K\033[0m\n");
        printf("\033[%d;1H", line_no++);
        printf("\n");
    }
    int visible = visible_content_rows();
    int printed = 0;
    int content_index = 0;

    for (size_t i = 0; i < num_lines && printed < visible; ++i)
    {
        int llen = (int)strlen(buffer[i]);
        int used = (llen == 0) ? 1 : ((llen + MAX_PRINTABLE - 1) / MAX_PRINTABLE);
        for (int chunk_off = 0; chunk_off < used && printed < visible; ++chunk_off)
        {
            if (content_index < screen_row_offset)
            {
                content_index++;
                continue;
            }
            int off = chunk_off * MAX_PRINTABLE;
            int chunk = llen - off;
            if (chunk > MAX_PRINTABLE)
                chunk = MAX_PRINTABLE;
            if (chunk < 0)
                chunk = 0;
            printf("\033[%d;1H", line_no++);
            if (chunk > 0)
            {
                if (syntax_highlight)
                {
                    int start = off;
                    int len = chunk;
                    char *tmp = malloc((size_t)len + 1);
                    if (!tmp)
                    {
                        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
                        exit(1);
                    }
                    strncpy(tmp, buffer[i] + start, len);
                    tmp[len] = '\0';
                    print_colored_c((int)i, tmp);
                    free(tmp);
                }
                else
                {
                    for (int j = 0; j < chunk; ++j)
                    {
                        if (is_selected((int)i, off + j))
                            printf("\033[1;37;46m%c\033[0m", buffer[i][off + j]);
                        else
                            putchar(buffer[i][off + j]);
                    }
                }
            }
            printf("\033[0K");
            printed++;
            content_index++;
        }
    }
    while (printed < visible)
    {
        printf("\033[%d;1H\033[0K", line_no++);
        printed++;
    }
    if (status_msg[0])
        printf("\033[%d;1H\033[0K\033[47;30m%s\033[0m", TERM_ROWS, status_msg);
    else
        printf("\033[%d;1H\033[0K", TERM_ROWS);

    int before = content_rows_before_logical(cur_row);
    int in_row_index = cur_col / MAX_PRINTABLE;
    int cursor_content_index = before + in_row_index;
    int screen_row_within = cursor_content_index - screen_row_offset;
    int screen_row = (welcome_active ? WELCOME_LINES : 0) + 1 + screen_row_within;
    int screen_col = (cur_col % MAX_PRINTABLE) + 1;
    if (screen_row >= TERM_ROWS)
        screen_row = TERM_ROWS - 1;
    if (screen_row < 1)
        screen_row = 1;
    if (screen_col < 1)
        screen_col = 1;
    if (screen_col > MAX_COLS)
        screen_col = MAX_COLS;
    printf("\033[%d;%dH", screen_row, screen_col);
    fflush(stdout);
}

static void shift_lines_down(int from)
{
    if (from < 0)
        return;
    ensure_buffer_rows((size_t)from);

    if (num_lines == 0)
        return;
    free(buffer[num_lines - 1]);

    for (size_t i = (size_t)num_lines - 1; i > from; --i)
    {
        buffer[i] = buffer[i - 1];
        line_sizes[i] = line_sizes[i - 1];
    }

    line_sizes[from] = default_line_size;
    buffer[from] = calloc(line_sizes[from], 1);
    if (!buffer[from])
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }
    buffer[from][0] = '\0';
}

static void shift_lines_up(int from)
{
    if (from < 0 || (size_t)from >= num_lines)
        return;

    free(buffer[from]);

    for (size_t i = from; i < num_lines - 1; ++i)
    {
        buffer[i] = buffer[i + 1];
        line_sizes[i] = line_sizes[i + 1];
    }

    line_sizes[num_lines - 1] = default_line_size;
    buffer[num_lines - 1] = calloc(line_sizes[num_lines - 1], 1);
    if (!buffer[num_lines - 1])
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        exit(1);
    }
    buffer[num_lines - 1][0] = '\0';
}

static void insert_newline_at_cursor(void)
{
    ensure_buffer_rows((size_t)cur_row + 1);

    char *row = buffer[cur_row];
    int len = (int)strlen(row);

    shift_lines_down(cur_row + 1);

    if (cur_col < len)
    {
        int right_len = len - cur_col;
        if (right_len > MAX_PRINTABLE)
            right_len = MAX_PRINTABLE;

        ensure_line_capacity(cur_row + 1, (size_t)right_len + 1);
        buffer[cur_row + 1][0] = '\0';
        strncpy(buffer[cur_row + 1], row + cur_col, (size_t)right_len);
        buffer[cur_row + 1][right_len] = '\0';

        row[cur_col] = '\0';
    }
    else
        buffer[cur_row + 1][0] = '\0';

    cur_row++;
    cur_col = 0;
    modified = 1;
    clear_status();
    reset_selection();
}

static void insert_char_at_cursor(int c)
{
    if (cur_row < 0)
        cur_row = 0;

    ensure_buffer_rows((size_t)cur_row);
    char *row = buffer[cur_row];
    int len = (int)strlen(row);

    int used_rows = (len + 1 + MAX_PRINTABLE - 1) / MAX_PRINTABLE;
    (void)used_rows;

    if (cur_col < 0)
        cur_col = 0;
    if (cur_col > len)
        cur_col = len;

    ensure_line_capacity(cur_row, (size_t)len + 2);

    for (int i = len; i >= cur_col; --i)
        row[i + 1] = row[i];

    row[cur_col] = (char)c;
    row[len + 1] = '\0';
    cur_col++;
    modified = 1;
    clear_status();
    reset_selection();
}

static void backspace_before_cursor(void)
{
    char *row = buffer[cur_row];
    int len = (int)strlen(row);
    if (cur_col > 0)
    {
        int i;
        for (i = cur_col - 1; i < len; ++i)
            row[i] = row[i + 1];
        cur_col--;
        modified = 1;
    }
    else
    {
        if (cur_row > 0)
        {
            int prev_len = (int)strlen(buffer[cur_row - 1]);
            int this_len = len;
            ensure_line_capacity(cur_row - 1, (size_t)prev_len + (size_t)this_len + 1);

            if (prev_len + this_len >= (int)line_sizes[cur_row - 1])
            {
                int copy_len = (int)line_sizes[cur_row - 1] - 1 - prev_len;
                if (copy_len > 0)
                    strncat(buffer[cur_row - 1], buffer[cur_row], (size_t)copy_len);
            }
            else
                strcat(buffer[cur_row - 1], buffer[cur_row]);
            shift_lines_up(cur_row);
            cur_row--;
            cur_col = prev_len;
            modified = 1;
        }
    }
    clear_status();
    reset_selection();
}

static void move_left(void)
{
    if (cur_col > 0)
        cur_col--;
    else if (cur_row > 0)
    {
        cur_row--;
        cur_col = (int)strlen(buffer[cur_row]);
    }
    clear_status();
    reset_selection();
}

static void move_right(void)
{
    int len = (int)strlen(buffer[cur_row]);
    if (cur_col < len)
    {
        cur_col++;
    }
    else
    {
        int last = get_last_filled_logical_row();
        if (cur_row < last)
        {
            cur_row++;
            cur_col = 0;
        }
    }
    clear_status();
    reset_selection();
}

static void move_up(void)
{
    if (cur_row > 0)
    {
        cur_row--;
        int len = (int)strlen(buffer[cur_row]);
        if (cur_col > len)
            cur_col = len;
    }
    clear_status();
    reset_selection();
}

static void move_down(void)
{
    int last = get_last_filled_logical_row();
    if (cur_row < last)
    {
        cur_row++;
        int len = (int)strlen(buffer[cur_row]);
        if (cur_col > len)
            cur_col = len;
    }
    clear_status();
    reset_selection();
}

static void copy_selection(void)
{
    clear_status();
    if (!selecting)
    {
        set_status(" Nothing to copy. ");
        return;
    }

    int start_row = sel_start_row, start_col = sel_start_col;
    int end_row = sel_end_row, end_col = sel_end_col;

    if (start_row > end_row || (start_row == end_row && start_col > end_col))
    {
        int tmp_row = start_row, tmp_col = start_col;
        start_row = end_row;
        start_col = end_col;
        end_row = tmp_row;
        end_col = tmp_col;
    }

    clipboard_reset();
    for (int r = start_row; r <= end_row; ++r)
    {
        int s_col = (r == start_row) ? start_col : 0;
        int e_col = (r == end_row) ? end_col : (int)strlen(buffer[r]);
        if (s_col >= e_col)
            continue;
        clipboard_append_text(buffer[r] + s_col, (size_t)(e_col - s_col));
        clipboard_append_text("\n", 1);
    }

    set_status(" Copied selection. ");
    reset_selection();
    editor_redraw();
}

static void cut_selection(void)
{
    clear_status();
    if (!selecting)
    {
        set_status(" Nothing to cut. ");
        return;
    }

    int start_row = sel_start_row, start_col = sel_start_col;
    int end_row = sel_end_row, end_col = sel_end_col;

    if (start_row > end_row || (start_row == end_row && start_col > end_col))
    {
        int tmp_row = start_row, tmp_col = start_col;
        start_row = end_row;
        start_col = end_col;
        end_row = tmp_row;
        end_col = tmp_col;
    }

    clipboard_reset();
    for (int r = start_row; r <= end_row; ++r)
    {
        int s_col = (r == start_row) ? start_col : 0;
        int e_col = (r == end_row) ? end_col : (int)strlen(buffer[r]);
        if (s_col >= e_col)
            continue;
        clipboard_append_text(buffer[r] + s_col, (size_t)(e_col - s_col));
        clipboard_append_text("\n", 1);
    }

    for (int r = start_row; r <= end_row; ++r)
    {
        int s_col = (r == start_row) ? start_col : 0;
        int e_col = (r == end_row) ? end_col : (int)strlen(buffer[r]);
        if (s_col >= e_col)
            continue;

        if (s_col == 0 && e_col == (int)strlen(buffer[r]))
        {
            shift_lines_up(r);
            end_row--;
            r--;
        }
        else
        {
            memmove(buffer[r] + s_col, buffer[r] + e_col, strlen(buffer[r]) - e_col + 1);
        }
    }

    cur_row = start_row;
    cur_col = start_col;
    modified = 1;
    reset_selection();
    set_status(" Cut selection. ");
    ensure_cursor_visible();
    editor_redraw();
}

static void paste_clipboard(void)
{
    clear_status();
    if (!clipboard || !clipboard[0])
    {
        set_status(" Nothing to paste. ");
        return;
    }

    int row = cur_row;
    int col = cur_col;
    char *clip = clipboard;

    while (*clip)
    {
        char *nl = strchr(clip, '\n');

        if (nl)
        {
            int len = (int)(nl - clip);

            ensure_buffer_rows((size_t)row);
            size_t curr_len = strlen(buffer[row]);
            ensure_line_capacity(row, curr_len + (size_t)len + 1);

            memmove(buffer[row] + col + len, buffer[row] + col, curr_len - col + 1);
            memcpy(buffer[row] + col, clip, (size_t)len);
            buffer[row][curr_len + len] = '\0';

            clip = nl + 1;

            if (*clip)
            {
                row++;
                ensure_buffer_rows((size_t)row);
                col = 0;
            }
            else
            {
                col += len;
                break;
            }
        }
        else
        {
            int len = (int)strlen(clip);
            ensure_buffer_rows((size_t)row);
            size_t curr_len = strlen(buffer[row]);
            ensure_line_capacity(row, curr_len + (size_t)len + 1);

            memmove(buffer[row] + col + len, buffer[row] + col, curr_len - col + 1);
            memcpy(buffer[row] + col, clip, (size_t)len);
            buffer[row][curr_len + len] = '\0';
            col += len;
            break;
        }
    }

    cur_row = row;
    cur_col = col;

    modified = 1;
    ensure_cursor_visible();
    editor_redraw();
}

static void editor_open_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return;

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return;
    }
    long fsz = ftell(f);
    if (fsz < 0)
    {
        fclose(f);
        return;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return;
    }

    size_t size = (size_t)fsz;
    unsigned char *tmp = NULL;
    if (size > 0)
    {
        tmp = malloc(size);
        if (!tmp)
        {
            fclose(f);
            return;
        }
        size_t r = fread(tmp, 1, size, f);
        fclose(f);
        if (r != size)
        {
            free(tmp);
            return;
        }
    }
    else
    {
        fclose(f);
        for (size_t i = 0; i < num_lines; ++i)
            buffer[i][0] = '\0';
        cur_row = cur_col = 0;
        modified = 0;
        clear_status();
        reset_selection();
        return;
    }

    int bin_hits = 0;
    for (size_t i = 0; i < size; ++i)
    {
        unsigned char c = tmp[i];
        if (c == 0)
            bin_hits += 4;
        else if (c < 32 && c != '\n' && c != '\r' && c != '\t')
            bin_hits++;
    }
    if (bin_hits > 0)
    {
        for (size_t i = 0; i < num_lines; ++i)
            buffer[i][0] = '\0';
        welcome_active = 0;
        readonly_mode = 1;
        set_status(" Not a text file. Press ESC to exit. ");
        editor_redraw();
        free(tmp);
        return;
    }

    for (size_t i = 0; i < num_lines; ++i)
        buffer[i][0] = '\0';

    int line = 0;
    int col = 0;
    for (size_t i = 0; i < size; ++i)
    {
        unsigned char c = tmp[i];
        if (c == '\r')
            continue;
        if (c == '\n')
        {
            ensure_buffer_rows((size_t)line);
            buffer[line][col] = '\0';
            line++;
            col = 0;
            if ((size_t)line >= num_lines)
                ensure_buffer_rows((size_t)line);
        }
        else
        {
            ensure_buffer_rows((size_t)line);
            if ((size_t)col + 1 >= line_sizes[line])
            {
                ensure_line_capacity(line, (size_t)col + 2);
            }
            buffer[line][col++] = (char)c;
        }
    }
    if (line < (int)num_lines && col > 0)
    {
        buffer[line][col] = '\0';
    }

    free(tmp);

    modified = 0;
    int last_line = 0;
    for (size_t i = 0; i < num_lines; ++i)
        if (buffer[i][0] != '\0')
            last_line = (int)i;
    cur_row = last_line;
    cur_col = (int)strlen(buffer[last_line]);
    clear_status();
    reset_selection();
    readonly_mode = 0;
}

static int editor_save_to_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return 0;
    int last_line = (int)num_lines - 1;
    while (last_line >= 0 && buffer[last_line][0] == '\0')
        last_line--;
    for (int i = 0; i <= last_line; ++i)
    {
        size_t len = strlen(buffer[i]);
        if (len > 0)
            fwrite(buffer[i], 1, len, f);
        fwrite("\n", 1, 1, f);
    }
    fclose(f);
    modified = 0;
    return 1;
}

static void prompt_filename_and_save(void)
{
    char namebuf[MAX_FILE_NAME + 1];
    while (1)
    {
        for (size_t i = 0; i < num_lines; ++i)
        {
            printf("\033[%u;1H", i + 1);
            printf("%s\033[0K", buffer[i]);
        }
        set_status(" Enter file name: ");
        editor_redraw();
        move_cursor_to_status_input();

        int pos = 0;
        int c;
        memset(namebuf, 0, sizeof(namebuf));

        while (1)
        {
            c = getchar();

            if (c == '\n' || c == '\r' || c == EOF)
                break;

            if (c == 8 || c == 127)
            {
                if (pos > 0)
                {
                    pos--;
                    namebuf[pos] = '\0';
                    printf("\b \b");
                    fflush(stdout);
                }
                continue;
            }

            if (pos >= MAX_FILE_NAME - 1)
                continue;

            if (isalnum(c) || c == '.')
            {
                namebuf[pos++] = (char)c;
                putchar(c);
                fflush(stdout);
            }
        }

        namebuf[pos] = '\0';

        if (pos == 0)
        {
            set_status(" File name cannot be empty. Press Enter to try again. ");
            editor_redraw();
            while ((c = getchar()) != '\n' && c != '\r' && c != EOF)
                ;
            continue;
        }

        strncpy(filename, namebuf, MAX_FILE_NAME);
        filename[MAX_FILE_NAME] = '\0';

        editor_save_to_file(filename);
        clear_status();
        reset_selection();
        editor_redraw();
        return;
    }
}

static void move_left_word_selection(void)
{
    if (!selecting)
    {
        sel_start_row = cur_row;
        sel_start_col = cur_col;
        selecting = 1;
    }

    char *row = buffer[cur_row];
    int i = cur_col;

    if (i > 0)
    {
        while (i > 0 && row[i - 1] == ' ')
            i--;
        while (i > 0 && row[i - 1] != ' ')
            i--;
    }
    else if (cur_row > 0)
    {
        cur_row--;
        cur_col = (int)strlen(buffer[cur_row]);
        move_left_word_selection();
        return;
    }

    cur_col = i;
    sel_end_row = cur_row;
    sel_end_col = cur_col;

    ensure_cursor_visible();
    editor_redraw();
}

static void move_right_word_selection(void)
{
    if (!selecting)
    {
        sel_start_row = cur_row;
        sel_start_col = cur_col;
        selecting = 1;
    }

    char *row = buffer[cur_row];
    int len = (int)strlen(row);
    int i = cur_col;

    if (i < len)
    {
        while (i < len && row[i] == ' ')
            i++;
        while (i < len && row[i] != ' ')
            i++;
    }
    else if (cur_row < get_last_filled_logical_row())
    {
        cur_row++;
        cur_col = 0;
        move_right_word_selection();
        return;
    }

    cur_col = i;
    sel_end_row = cur_row;
    sel_end_col = cur_col;

    ensure_cursor_visible();
    editor_redraw();
}

static void move_up_word_selection(void)
{
    if (!selecting)
    {
        sel_start_row = cur_row;
        sel_start_col = cur_col;
        selecting = 1;
    }

    if (cur_row > 0)
    {
        cur_row--;
        int len = (int)strlen(buffer[cur_row]);
        if (cur_col > len)
            cur_col = len;
    }

    sel_end_row = cur_row;
    sel_end_col = cur_col;

    ensure_cursor_visible();
    editor_redraw();
}

static void move_down_word_selection(void)
{
    if (!selecting)
    {
        sel_start_row = cur_row;
        sel_start_col = cur_col;
        selecting = 1;
    }

    int last = get_last_filled_logical_row();
    if (cur_row < last)
    {
        cur_row++;
        int len = (int)strlen(buffer[cur_row]);
        if (cur_col > len)
            cur_col = len;
    }

    sel_end_row = cur_row;
    sel_end_col = cur_col;

    ensure_cursor_visible();
    editor_redraw();
}

int main(int argc, char **argv)
{
    editor_init();

    cur_row = 0;
    cur_col = 0;
    modified = 0;
    welcome_active = 1;

    if (argc >= 2)
    {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';

        if (strlen(filename) > 2 && strcmp(filename + strlen(filename) - 2, ".c") == 0)
            syntax_highlight = 1;

        editor_open_file(filename);
        ensure_cursor_visible();
    }
    else
    {
        strncpy(filename, "NO NAME", sizeof(filename) - 1);
    }

    ensure_cursor_visible();
    editor_redraw();

    int running = 1;
    while (running)
    {
        int c = getchar();
        if (readonly_mode)
        {
            if (c == 27)
            {
                running = 0;
                break;
            }
            else
                continue;
        }
        if (c == EOF)
            break;
        if (c == 1)
            move_up();
        if (c == 2)
            move_down();
        if (c == 3)
            move_left();
        if (c == 4)
            move_right();
        if (c == 27)
        {
            if (!modified)
            {
                running = 0;
                break;
            }

            set_status(" Save file? (y - yes, n - no) ");
            ensure_cursor_visible();
            editor_redraw();
            while (1)
            {
                int yn = getchar();
                if (yn == EOF)
                {
                    running = 0;
                    break;
                }
                if (yn == 'y' || yn == 'Y')
                {
                    if (strcmp(filename, "NO NAME") == 0)
                        prompt_filename_and_save();
                    else
                        editor_save_to_file(filename);

                    running = 0;
                    break;
                }
                else if (yn == 'n' || yn == 'N')
                {
                    running = 0;
                    break;
                }
                else
                    continue;
            }
            clear_status();
            reset_selection();
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == '\r' || c == '\n')
        {
            insert_newline_at_cursor();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 8)
        {
            backspace_before_cursor();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 127)
        {
            char *row = buffer[cur_row];
            int len = (int)strlen(row);
            if (cur_col < len)
            {
                for (int i = cur_col; i < len; ++i)
                    row[i] = row[i + 1];
                modified = 1;
            }
            clear_status();
            reset_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 128)
        {
            char *row = buffer[cur_row];
            int len = (int)strlen(row);
            if (cur_col > 0 && cur_col <= len)
            {
                memmove(row, row + cur_col, (size_t)(len - cur_col + 1));
                cur_col = 0;
                modified = 1;
            }
            clear_status();
            reset_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 129)
        {
            char *row = buffer[cur_row];
            int len = (int)strlen(row);
            if (cur_col > 0)
            {
                int i = cur_col;
                while (i > 0 && row[i - 1] == ' ')
                    i--;
                while (i > 0 && row[i - 1] != ' ')
                    i--;
                memmove(row + i, row + cur_col, (size_t)(len - cur_col + 1));
                cur_col = i;
                modified = 1;
            }
            clear_status();
            reset_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 130)
        {
            char *row = buffer[cur_row];
            int len = (int)strlen(row);
            if (cur_col < len)
            {
                row[cur_col] = '\0';
                modified = 1;
            }
            clear_status();
            reset_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 131)
        {
            char *row = buffer[cur_row];
            int len = (int)strlen(row);
            if (cur_col < len)
            {
                int j = cur_col;
                while (j < len && row[j] == ' ')
                    j++;
                while (j < len && row[j] != ' ')
                    j++;
                memmove(row + cur_col, row + j, (size_t)(len - j + 1));
                modified = 1;
            }
            clear_status();
            reset_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 132)
        {
            if (!selecting)
            {
                sel_start_row = cur_row;
                sel_start_col = cur_col;
                selecting = 1;
            }

            if (cur_col > 0)
                cur_col--;
            else if (cur_row > 0)
            {
                cur_row--;
                cur_col = (int)strlen(buffer[cur_row]);
            }

            sel_end_row = cur_row;
            sel_end_col = cur_col;

            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 133)
        {
            if (!selecting)
            {
                sel_start_row = cur_row;
                sel_start_col = cur_col;
                selecting = 1;
            }

            int len = (int)strlen(buffer[cur_row]);
            if (cur_col < len)
                cur_col++;
            else if (cur_row < get_last_filled_logical_row())
            {
                cur_row++;
                cur_col = 0;
            }

            sel_end_row = cur_row;
            sel_end_col = cur_col;

            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 134)
        {
            if (!selecting)
            {
                sel_start_row = cur_row;
                sel_start_col = cur_col;
                selecting = 1;
            }

            if (cur_row > 0)
            {
                cur_row--;
                int len = (int)strlen(buffer[cur_row]);
                if (cur_col > len)
                    cur_col = len;
            }

            sel_end_row = cur_row;
            sel_end_col = cur_col;

            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 135)
        {
            if (!selecting)
            {
                sel_start_row = cur_row;
                sel_start_col = cur_col;
                selecting = 1;
            }

            int last = get_last_filled_logical_row();
            if (cur_row < last)
            {
                cur_row++;
                int len = (int)strlen(buffer[cur_row]);
                if (cur_col > len)
                    cur_col = len;
            }

            sel_end_row = cur_row;
            sel_end_col = cur_col;

            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 136)
            move_left_word_selection();
        else if (c == 137)
            move_right_word_selection();
        else if (c == 138)
            move_up_word_selection();
        else if (c == 139)
            move_down_word_selection();
        else if (c == 140)
        {
            cut_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 141)
        {
            copy_selection();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == 142)
        {
            paste_clipboard();
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c == '\t')
        {
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else if (c >= 32 && c <= 126)
        {
            insert_char_at_cursor(c);
            welcome_active = 0;
            ensure_cursor_visible();
            editor_redraw();
        }
        else
        {
            ensure_cursor_visible();
            editor_redraw();
        }
    }

    clear_screen_and_home();

    for (size_t i = 0; i < num_lines; ++i)
        free(buffer[i]);

    free(buffer);
    free(line_sizes);
    free(clipboard);

    return 0;
}
