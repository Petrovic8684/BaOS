#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_LINES 25
#define MAX_COLS 80
#define WELCOME_LINES 2
#define TERM_ROWS 25

static char buffer[MAX_LINES][MAX_COLS];
static char filename[128] = "NO NAME";
static int cur_row = 0, cur_col = 0;
static int modified = 0;
static int welcome_active = 1;
static int readonly_mode = 0;
static char status_msg[MAX_COLS] = "";

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
    int i, j;
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

    for (i = 0; i < MAX_LINES; ++i, ++line_no)
    {
        printf("\033[%d;1H", line_no);

        int len = (int)strlen(buffer[i]);
        if (len > MAX_COLS - 1)
            len = MAX_COLS - 1;

        if (len > 0)
        {
            for (j = 0; j < len; ++j)
                putchar(buffer[i][j]);
        }

        printf("\033[0K");
    }

    int offset = welcome_active ? WELCOME_LINES : 0;
    int screen_row = offset + cur_row + 1;
    int screen_col = cur_col + 1;
    if (screen_row >= TERM_ROWS)
        screen_row = TERM_ROWS - 1;
    if (screen_row < 1)
        screen_row = 1;
    if (screen_col < 1)
        screen_col = 1;
    if (screen_col > MAX_COLS)
        screen_col = MAX_COLS;

    if (status_msg[0])
        printf("\033[%d;1H\033[0K\033[47;30m%s\033[0m", TERM_ROWS, status_msg);
    else
        printf("\033[%d;1H\033[0K", TERM_ROWS);

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
        memcpy(buffer[i], buffer[i - 1], MAX_COLS);
        buffer[i][MAX_COLS - 1] = '\0';
    }
    memset(buffer[from], 0, MAX_COLS);
    buffer[from][MAX_COLS - 1] = '\0';
}

static void shift_lines_up(int from)
{
    int i;
    if (from < 0 || from >= MAX_LINES)
        return;
    for (i = from; i < MAX_LINES - 1; ++i)
    {
        memcpy(buffer[i], buffer[i + 1], MAX_COLS);
        buffer[i][MAX_COLS - 1] = '\0';
    }
    memset(buffer[MAX_LINES - 1], 0, MAX_COLS);
    buffer[MAX_LINES - 1][MAX_COLS - 1] = '\0';
}

static void insert_newline_at_cursor(void)
{
    char *row = buffer[cur_row];
    int len = (int)strlen(row);
    if (cur_row >= MAX_LINES - 1)
    {
        cur_col = len;
        return;
    }
    shift_lines_down(cur_row + 1);
    if (cur_col < len)
    {
        int right_len = len - cur_col;
        if (right_len >= MAX_COLS)
            right_len = MAX_COLS - 1;
        memset(buffer[cur_row + 1], 0, MAX_COLS);
        strncpy(buffer[cur_row + 1], row + cur_col, right_len);
        buffer[cur_row + 1][right_len] = '\0';
        row[cur_col] = '\0';
    }
    else
        memset(buffer[cur_row + 1], 0, MAX_COLS);
    cur_row++;
    cur_col = 0;
    modified = 1;
}

static void insert_char_at_cursor(int c)
{
    char *row = buffer[cur_row];
    int len = (int)strlen(row);

    if (cur_col >= MAX_COLS - 1)
    {
        if (cur_row < MAX_LINES - 1)
        {
            insert_newline_at_cursor();
            buffer[cur_row][cur_col] = (char)c;
            buffer[cur_row][cur_col + 1] = '\0';
            cur_col++;
            modified = 1;
        }
        return;
    }

    if (len >= MAX_COLS - 1)
    {
        if (cur_row < MAX_LINES - 1)
        {
            insert_newline_at_cursor();
            buffer[cur_row][cur_col] = (char)c;
            buffer[cur_row][cur_col + 1] = '\0';
            cur_col++;
            modified = 1;
        }
        return;
    }

    if (cur_col < 0)
        cur_col = 0;
    if (cur_col > len)
        cur_col = len;

    for (int i = len; i >= cur_col; --i)
        row[i + 1] = row[i];

    row[cur_col] = (char)c;
    row[len + 1] = '\0';
    cur_col++;
    modified = 1;
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
            if (prev_len + this_len >= MAX_COLS)
            {
                int copy_len = MAX_COLS - 1 - prev_len;
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
}

static void move_right(void)
{
    int len = (int)strlen(buffer[cur_row]);
    if (cur_col < len)
        cur_col++;
    else if (cur_row < MAX_LINES - 1)
    {
        cur_row++;
        cur_col = 0;
    }
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
}

static void move_down(void)
{
    if (cur_row < MAX_LINES - 1)
    {
        cur_row++;
        int len = (int)strlen(buffer[cur_row]);
        if (cur_col > len)
            cur_col = len;
    }
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
        if (c == '\n' || col >= MAX_COLS - 1)
        {
            buffer[line][col] = '\0';
            line++;
            col = 0;
        }
        else
            buffer[line][col++] = (char)c;
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
        editor_open_file(filename);
    }
    else
        strncpy(filename, "NO NAME", sizeof(filename) - 1);

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
            set_status(" Save file? (y - yes, n - no) ");
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
                    {
                        prompt_filename_and_save();
                    }
                    else
                    {
                        editor_save_to_file(filename);
                    }
                    running = 0;
                    break;
                }
                else if (yn == 'n' || yn == 'N')
                {
                    running = 0;
                    break;
                }
                else
                {
                    continue;
                }
            }
            clear_status();
            editor_redraw();
        }
        else if (c == '\r' || c == '\n')
        {
            insert_newline_at_cursor();
            welcome_active = 0;
            editor_redraw();
        }
        else if (c == 8)
        {
            backspace_before_cursor();
            welcome_active = 0;
            editor_redraw();
        }
        else if (c == '\t')
        {
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            insert_char_at_cursor(' ');
            welcome_active = 0;
            editor_redraw();
        }
        else if (c >= 32 && c <= 126)
        {
            insert_char_at_cursor(c);
            welcome_active = 0;
            editor_redraw();
        }
        else
            editor_redraw();
    }

    clear_screen_and_home();
    return 0;
}