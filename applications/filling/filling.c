#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_LINES 200
#define MAX_COLS 80
#define WELCOME_LINES 2
#define TERM_ROWS 25
#define MAX_PRINTABLE (MAX_COLS)
#define STORAGE_COLS (2048)

static char buffer[MAX_LINES][STORAGE_COLS];
static char filename[128] = "NO NAME";
static int cur_row = 0, cur_col = 0;
static int modified = 0;
static int welcome_active = 1;
static int readonly_mode = 0;
static char status_msg[MAX_COLS] = "";
static int screen_row_offset = 0;
static int syntax_highlight = 0;

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
    int len = (int)strlen(buffer[logical_row]);
    if (len == 0)
        return 1;
    return (len + MAX_PRINTABLE - 1) / MAX_PRINTABLE;
}

static int total_content_rows(void)
{
    int total = 0;
    for (int i = 0; i < MAX_LINES; ++i)
        total += content_rows_used_by_line(i);
    return total;
}

static int content_rows_before_logical(int logical_row)
{
    int total = 0;
    for (int i = 0; i < logical_row && i < MAX_LINES; ++i)
        total += content_rows_used_by_line(i);
    return total;
}

static int get_last_filled_logical_row(void)
{
    for (int i = MAX_LINES - 1; i >= 0; --i)
        if (buffer[i][0] != '\0')
            return i;
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

static void print_colored_c(const char *line)
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

        if (in_comment)
        {
            putchar(c);
            if (c == '\n')
                in_comment = 0;
        }
        else if (in_string)
        {
            printf("\033[1;32m%c\033[0m", c);
            if (c == '"' && line[i - 1] != '\\')
                in_string = 0;
        }
        else if (in_char)
        {
            printf("\033[1;32m%c\033[0m", c);
            if (c == '\'' && line[i - 1] != '\\')
                in_char = 0;
        }
        else
        {
            if (c == '/' && line[i + 1] == '/')
            {
                printf("\033[32m%s\033[0m", line + i);
                break;
            }
            else if (c == '"')
            {
                in_string = 1;
                printf("\033[1;32m%c\033[0m", c);
            }
            else if (c == '\'')
            {
                in_char = 1;
                printf("\033[1;32m%c\033[0m", c);
            }
            else if (isdigit(c))
            {
                int start = i;
                while (isdigit(line[i]) || line[i] == '.' || line[i] == 'x' || (line[i] >= 'a' && line[i] <= 'f') || (line[i] >= 'A' && line[i] <= 'F'))
                    i++;
                char num[32];
                int len = i - start;
                strncpy(num, line + start, len);
                num[len] = '\0';
                printf("\033[1;35m%s\033[0m", num);
                i--;
            }
            else if (isalpha(c) || c == '_')
            {
                int start = i;
                while (isalnum(line[i]) || line[i] == '_')
                    i++;
                int len = i - start;
                i--;
                char word[64];
                if (len < 64)
                {
                    strncpy(word, line + start, len);
                    word[len] = '\0';

                    int is_kw = 0;
                    for (int k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
                        if (strcmp(word, keywords[k]) == 0)
                        {
                            is_kw = 1;
                            break;
                        }

                    if (is_kw)
                    {
                        if (strcmp(word, "const") == 0 || strcmp(word, "static") == 0)
                            printf("\033[33m%s\033[0m", word);
                        else if (strcmp(word, "sizeof") == 0)
                            printf("\033[1;32m%s\033[0m", word);
                        else
                            printf("\033[31m%s\033[0m", word);
                    }
                    else
                    {
                        int j = i + 1;
                        while (line[j] == ' ' || line[j] == '\t')
                            j++;
                        if (line[j] == '(')
                            printf("\033[1;33m%s\033[0m", word);
                        else
                            printf("\033[1;34m%s\033[0m", word);
                    }
                }
            }
            else
                putchar(c);
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
    int len = (int)strlen(buffer[row]);
    int new_len = len + 1;
    int used_rows = (new_len + MAX_PRINTABLE - 1) / MAX_PRINTABLE;
    return row + used_rows > MAX_LINES - 1;
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
    for (int i = 0; i < MAX_LINES && printed < visible; ++i)
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
                    char tmp[STORAGE_COLS];
                    strncpy(tmp, buffer[i] + start, len);
                    tmp[len] = '\0';
                    print_colored_c(tmp);
                }
                else
                {
                    for (int j = 0; j < chunk; ++j)
                        putchar(buffer[i][off + j]);
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
    int i;
    if (from < 0 || from >= MAX_LINES)
        return;
    for (i = MAX_LINES - 1; i > from; --i)
    {
        memcpy(buffer[i], buffer[i - 1], STORAGE_COLS);
        buffer[i][STORAGE_COLS - 1] = '\0';
    }
    memset(buffer[from], 0, STORAGE_COLS);
    buffer[from][STORAGE_COLS - 1] = '\0';
}

static void shift_lines_up(int from)
{
    int i;
    if (from < 0 || from >= MAX_LINES)
        return;
    for (i = from; i < MAX_LINES - 1; ++i)
    {
        memcpy(buffer[i], buffer[i + 1], STORAGE_COLS);
        buffer[i][STORAGE_COLS - 1] = '\0';
    }
    memset(buffer[MAX_LINES - 1], 0, STORAGE_COLS);
    buffer[MAX_LINES - 1][STORAGE_COLS - 1] = '\0';
}

static void insert_newline_at_cursor(void)
{
    char *row = buffer[cur_row];
    int len = (int)strlen(row);

    if (cur_row >= MAX_LINES - 1)
    {
        cur_col = len;
        set_status(" Reached maximum number of lines. ");
        return;
    }

    shift_lines_down(cur_row + 1);

    if (cur_col < len)
    {
        int right_len = len - cur_col;
        if (right_len > MAX_PRINTABLE)
            right_len = MAX_PRINTABLE;

        if (cur_row + 1 >= MAX_LINES - 1)
        {
            cur_col = len;
            set_status(" Reached maximum number of lines. ");
            return;
        }

        memset(buffer[cur_row + 1], 0, STORAGE_COLS);
        strncpy(buffer[cur_row + 1], row + cur_col, right_len);
        buffer[cur_row + 1][right_len] = '\0';
        row[cur_col] = '\0';
    }
    else
        memset(buffer[cur_row + 1], 0, STORAGE_COLS);

    cur_row++;
    cur_col = 0;
    modified = 1;
    clear_status();
}

static void insert_char_at_cursor(int c)
{
    if (cur_row < 0)
        cur_row = 0;

    char *row = buffer[cur_row];
    int len = (int)strlen(row);

    int used_rows = (len + 1 + MAX_PRINTABLE - 1) / MAX_PRINTABLE;
    if (cur_row + used_rows > MAX_LINES - 1)
    {
        set_status(" Reached maximum number of lines. ");
        return;
    }

    if (cur_col < 0)
        cur_col = 0;
    if (cur_col > len)
        cur_col = len;
    if (len >= STORAGE_COLS - 1)
        return;

    for (int i = len; i >= cur_col; --i)
        row[i + 1] = row[i];

    row[cur_col] = (char)c;
    row[len + 1] = '\0';
    cur_col++;
    modified = 1;
    clear_status();
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
            if (prev_len + this_len >= STORAGE_COLS - 1)
            {
                int copy_len = (STORAGE_COLS - 1) - prev_len;
                if (copy_len > 0)
                    strncat(buffer[cur_row - 1], buffer[cur_row], copy_len);
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
}

static void editor_open_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    for (int i = 0; i < MAX_LINES; ++i)
        buffer[i][0] = '\0';
    unsigned char tmp[8192];
    size_t r = fread(tmp, 1, sizeof(tmp), f);
    fclose(f);
    int bin_hits = 0;
    for (size_t i = 0; i < r; ++i)
    {
        unsigned char c = tmp[i];
        if (c == 0)
            bin_hits += 4;
        else if (c < 32 && c != '\n' && c != '\r' && c != '\t')
            bin_hits++;
    }
    if (bin_hits > 0)
    {
        for (int i = 0; i < MAX_LINES; ++i)
            buffer[i][0] = '\0';
        welcome_active = 0;
        readonly_mode = 1;
        set_status(" Not a text file. Press ESC to exit. ");
        editor_redraw();
        return;
    }
    int line = 0;
    int col = 0;
    for (size_t i = 0; i < r && line < MAX_LINES; ++i)
    {
        unsigned char c = tmp[i];
        if (c == '\r')
            continue;
        if (c == '\n')
        {
            buffer[line][col] = '\0';
            line++;
            col = 0;
        }
        else
        {
            if (col < STORAGE_COLS - 1)
                buffer[line][col++] = (char)c;
        }
    }
    if (line < MAX_LINES && col > 0)
        buffer[line][col] = '\0';
    modified = 0;
    int last_line = 0;
    for (int i = 0; i < MAX_LINES; ++i)
        if (buffer[i][0] != '\0')
            last_line = i;
    cur_row = last_line;
    cur_col = (int)strlen(buffer[last_line]);
    clear_status();
    readonly_mode = 0;
}

static int editor_save_to_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return 0;
    int last_line = MAX_LINES - 1;
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
    char namebuf[128];
    while (1)
    {
        for (int i = 0; i < MAX_LINES; ++i)
        {
            printf("\033[%d;1H", i + 1);
            printf("%s\033[0K", buffer[i]);
        }
        set_status(" Enter file name: ");
        editor_redraw();
        move_cursor_to_status_input();
        int pos = 0;
        int c;
        memset(namebuf, 0, sizeof(namebuf));
        while ((c = getchar()) != '\n' && c != '\r' && c != EOF && pos < (int)sizeof(namebuf) - 1)
        {
            if (c == 8)
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
            namebuf[pos++] = (char)c;
            putchar(c);
            fflush(stdout);
        }
        namebuf[pos] = '\0';
        int start = 0;
        while (namebuf[start] && isspace((unsigned char)namebuf[start]))
            start++;
        int end = (int)strlen(namebuf) - 1;
        while (end >= start && isspace((unsigned char)namebuf[end]))
        {
            namebuf[end] = '\0';
            end--;
        }
        if (namebuf[start] == '\0')
        {
            set_status(" File name cannot be empty. Press Enter to try again. ");
            editor_redraw();
            while ((c = getchar()) != '\n' && c != '\r' && c != EOF)
                ;
            continue;
        }
        else
        {
            strncpy(filename, namebuf + start, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
            editor_save_to_file(filename);
            clear_status();
            editor_redraw();
            return;
        }
    }
}

int main(int argc, char **argv)
{
    for (int i = 0; i < MAX_LINES; ++i)
        buffer[i][0] = '\0';
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
        strncpy(filename, "NO NAME", sizeof(filename) - 1);
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
    return 0;
}