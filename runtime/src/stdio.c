#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_FS_MAKE_FILE 13
#define SYS_FS_DELETE_FILE 14
#define SYS_FS_WRITE_FILE 15
#define SYS_FS_READ_FILE 16
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19

#define MAX_OPEN_FILES 16

#define TTY_ROWS 25
#define TTY_COLS 80
#define TTY_LAST_ROW (TTY_ROWS - 1)
#define TTY_LAST_COL (TTY_COLS - 1)

static inline void sys_write(const char *str)
{
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline unsigned char sys_read(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_READ)
        : "eax", "ebx", "memory");
    return (unsigned char)(ret & 0xFF);
}

static inline int fs_make_file(const char *name)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_MAKE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_delete_file(const char *name)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_write_file(const char *name, const unsigned char *data, unsigned int size)
{
    struct
    {
        const char *name;
        const unsigned char *data;
        unsigned int size;
    } args = {name, data, size};

    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_WRITE_FILE), [a] "r"(&args)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size)
{
    struct
    {
        const char *name;
        unsigned char *out_buf;
        unsigned int buf_size;
        unsigned int *out_size;
    } args = {name, out_buf, buf_size, out_size};

    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_READ_FILE), [a] "r"(&args)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_read_file_size(const char *name)
{
    unsigned int out = 0;
    int rc = fs_read_file(name, NULL, 0, &out);
    return (rc == 0) ? (int)out : -1;
}

static inline int sys_get_cursor_row(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_ROW)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_get_cursor_col(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_COL)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static int tty_row = 0;
static int tty_col = 0;
static unsigned char tty_attr = 0x07;

enum ansi_state
{
    ANSI_NORMAL_ST = 0,
    ANSI_ESC_ST,
    ANSI_CSI_ST
};

static int ansi_state = ANSI_NORMAL_ST;
static char ansi_params[64];
static int ansi_params_len = 0;

void write(const char *str);

static char *int_to_dec_str(unsigned int v, char *out_end)
{
    char buf[16];
    int i = 0;
    if (v == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        while (v > 0 && i < (int)sizeof(buf))
        {
            buf[i++] = '0' + (v % 10);
            v /= 10;
        }
    }
    char *p = out_end;
    while (i-- > 0)
        *p++ = buf[i];
    *p = '\0';
    return out_end;
}

static void send_move_syscall(int r, int c)
{
    char seq[32];
    char *p = seq;
    *p++ = 0x1B;
    *p++ = '[';
    char numbuf[12];
    int_to_dec_str((unsigned int)r, numbuf);
    for (char *q = numbuf; *q; q++)
        *p++ = *q;
    *p++ = ';';
    int_to_dec_str((unsigned int)c, numbuf);
    for (char *q = numbuf; *q; q++)
        *p++ = *q;
    *p++ = 'H';
    *p = '\0';
    write(seq);
}

static void scroll_up(int n)
{
    if (n <= 0)
        return;

    char seq[16];
    char *p = seq;
    *p++ = 0x1B;
    *p++ = '[';
    char numbuf[12];
    int_to_dec_str((unsigned int)n, numbuf);
    for (char *q = numbuf; *q; q++)
        *p++ = *q;
    *p++ = 'S';
    *p = '\0';

    write(seq);

    int r = sys_get_cursor_row();
    int c = sys_get_cursor_col();
    if (r < 0)
        r = 0;
    if (r > TTY_LAST_ROW)
        r = TTY_LAST_ROW;
    if (c < 0)
        c = 0;
    if (c > TTY_LAST_COL)
        c = TTY_LAST_COL;

    tty_row = r;
    tty_col = c;

    send_move_syscall(tty_row + 1, tty_col + 1);
}

static void tty_wrap_char(unsigned char ch)
{
    if (ch == '\n')
    {
        tty_col = 0;
        tty_row++;
        if (tty_row > TTY_LAST_ROW)
            tty_row = TTY_LAST_ROW;
        return;
    }
    else if (ch == '\r')
    {
        tty_col = 0;
        return;
    }
    else
    {
        tty_col++;
        if (tty_col >= TTY_COLS)
        {
            tty_col = 0;
            tty_row++;
            if (tty_row > TTY_LAST_ROW)
                tty_row = TTY_LAST_ROW;
        }
    }
}

static void tty_parse_after_write(const char *s)
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
                    if (tty_row < 0)
                        tty_row = 0;
                    if (tty_col < 0)
                        tty_col = 0;
                    if (tty_row > TTY_LAST_ROW)
                        tty_row = TTY_LAST_ROW;
                    if (tty_col > TTY_LAST_COL)
                        tty_col = TTY_LAST_COL;
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

void write(const char *str)
{
    if (!str)
        return;

    sys_write(str);
    tty_parse_after_write(str);

    int real_row = sys_get_cursor_row();
    int real_col = sys_get_cursor_col();

    if (real_row < 0)
        real_row = 0;
    if (real_col < 0)
        real_col = 0;
    if (real_row > TTY_LAST_ROW)
        real_row = TTY_LAST_ROW;
    if (real_col > TTY_LAST_COL)
        real_col = TTY_LAST_COL;

    tty_row = real_row;
    tty_col = real_col;
}

static void update_cursor(int new_row, int new_col)
{
    if (new_col < 0)
        new_col = 0;

    if (new_col >= TTY_COLS)
    {
        new_row += new_col / TTY_COLS;
        new_col = new_col % TTY_COLS;
    }

    if (new_row < 0)
        new_row = 0;

    if (new_row > TTY_LAST_ROW)
        new_row = TTY_LAST_ROW;

    if (new_col < 0)
        new_col = 0;
    if (new_col > TTY_LAST_COL)
        new_col = TTY_LAST_COL;

    tty_row = new_row;
    tty_col = new_col;
    send_move_syscall(tty_row + 1, tty_col + 1);
}

static void redraw_buffer(const char *buf, int len, int start_row, int start_col)
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
    if (after_row < 0)
        after_row = 0;
    if (after_col < 0)
        after_col = 0;
    if (after_row > TTY_LAST_ROW)
        after_row = TTY_LAST_ROW;
    if (after_col > TTY_LAST_COL)
        after_col = TTY_LAST_COL;

    send_move_syscall(after_row + 1, after_col + 1);

    write("\x1B[K");

    tty_row = after_row;
    tty_col = after_col;
}

static FILE _file_table[MAX_OPEN_FILES];

static FILE *alloc_file_slot(void)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (_file_table[i].name == NULL)
        {
            _file_table[i].mode = 0;
            _file_table[i].pos = 0;
            _file_table[i].buf = NULL;
            _file_table[i].buf_pos = 0;
            _file_table[i].buf_end = 0;
            _file_table[i].eof = 0;
            _file_table[i].err = 0;
            return &_file_table[i];
        }
    }
    return NULL;
}

static void free_file_slot(FILE *f)
{
    if (!f)
        return;

    if (f->buf)
    {
        free(f->buf);
        f->buf = NULL;
    }

    if (f->name)
    {
        free((void *)f->name);
        f->name = NULL;
    }

    f->mode = 0;
    f->pos = 0;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->eof = 0;
    f->err = 0;
}

static int stdin_ungetc = -1;

int fputc(int c, FILE *stream)
{
    if (!stream)
        return EOF;

    if (stream == stdout || stream == stderr)
    {
        char tmp[2] = {(char)c, 0};
        write(tmp);
        return c;
    }

    if (!stream->buf)
    {
        stream->buf = (unsigned char *)malloc(128);
        if (!stream->buf)
        {
            stream->err = 1;
            return EOF;
        }
        stream->buf_pos = 0;
        stream->buf_end = 128;
    }
    else if (stream->buf_pos >= stream->buf_end)
    {
        unsigned char *new_buf = (unsigned char *)realloc(stream->buf, stream->buf_end * 2);
        if (!new_buf)
        {
            stream->err = 1;
            return EOF;
        }
        stream->buf = new_buf;
        stream->buf_end *= 2;
    }

    stream->buf[stream->buf_pos++] = (unsigned char)c;
    return c;
}

int putchar(int c) { return fputc(c, stdout); }

int fputs(const char *s, FILE *stream)
{
    while (*s)
        fputc(*s++, stream);
    return 0;
}

int puts(const char *s)
{
    fputs(s, stdout);
    fputc('\n', stdout);
    return 0;
}

int vfprintf(FILE *stream, const char *fmt, va_list args)
{
    char tmp[64];
    int total = 0;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%' && *(p + 1))
        {
            p++;
            int long_flag = 0;
            if (*p == 'l')
            {
                long_flag = 1;
                p++;
            }

            if (*p == 's')
            {
                const char *s = va_arg(args, const char *);
                while (*s)
                {
                    if (fputc((unsigned char)*s++, stream) == EOF)
                        return -1;
                    total++;
                }
            }
            else if (*p == 'c')
            {
                char ch = (char)va_arg(args, int);
                if (fputc(ch, stream) == EOF)
                    return -1;
                total++;
            }
            else if (*p == 'd')
            {
                long val = long_flag ? va_arg(args, long) : va_arg(args, int);
                int neg = 0;
                int i = 0;
                if (val == 0)
                {
                    fputc('0', stream);
                    total++;
                    continue;
                }
                if (val < 0)
                {
                    neg = 1;
                    val = -val;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                if (neg)
                {
                    fputc('-', stream);
                    total++;
                }
                while (i-- > 0)
                {
                    fputc(tmp[i], stream);
                    total++;
                }
            }
            else if (*p == 'u')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                if (val == 0)
                {
                    fputc('0', stream);
                    total++;
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                while (i-- > 0)
                {
                    fputc(tmp[i], stream);
                    total++;
                }
            }
            else if (*p == 'x' || *p == 'X')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                const char *digits = (*p == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                if (val == 0)
                {
                    fputc('0', stream);
                    total++;
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                {
                    fputc(tmp[i], stream);
                    total++;
                }
            }
            else if (*p == 'p')
            {
                void *ptr = va_arg(args, void *);
                unsigned long val = (unsigned long)ptr;
                fputs("0x", stream);
                total += 2;
                int i = 0;
                const char *digits = "0123456789abcdef";
                if (val == 0)
                {
                    fputc('0', stream);
                    total++;
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                {
                    fputc(tmp[i], stream);
                    total++;
                }
            }
            else if (*p == 'f' || *p == 'g')
            {
                double val = va_arg(args, double);
                if (val < 0)
                {
                    fputc('-', stream);
                    total++;
                    val = -val;
                }

                long int_part = (long)val;
                double frac_part = val - int_part;
                int i = 0;
                if (int_part == 0)
                    tmp[i++] = '0';
                else
                {
                    long tmpval = int_part;
                    char tmpbuf[32];
                    int j = 0;
                    while (tmpval > 0)
                    {
                        tmpbuf[j++] = '0' + tmpval % 10;
                        tmpval /= 10;
                    }
                    while (j-- > 0)
                        tmp[i++] = tmpbuf[j];
                }
                tmp[i] = 0;
                fputs(tmp, stream);
                total += i;

                fputc('.', stream);
                total++;
                for (int d = 0; d < 10; d++)
                {
                    frac_part *= 10;
                    int digit = (int)frac_part;
                    fputc('0' + digit, stream);
                    total++;
                    frac_part -= digit;
                }
            }
            else if (*p == '%')
            {
                fputc('%', stream);
                total++;
            }
            else
            {
                fputc('%', stream);
                fputc(*p, stream);
                total += 2;
            }
        }
        else
        {
            fputc(*p, stream);
            total++;
        }
    }

    return total;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stream, fmt, args);
    va_end(args);
    return ret;
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stdout, fmt, args);
    va_end(args);
    return ret;
}

int getchar(void)
{
    if (stdin_ungetc != -1)
    {
        int c = stdin_ungetc;
        stdin_ungetc = -1;
        return c;
    }
    return (int)sys_read();
}

int ungetc(int c, FILE *stream)
{
    if (stream == stdin)
    {
        stdin_ungetc = c & 0xFF;
        return c;
    }
    if (!stream || stream->buf_pos == 0)
        return EOF;

    stream->buf[--stream->buf_pos] = (unsigned char)c;
    return c;
}

int fgetc(FILE *stream)
{
    if (!stream)
        return EOF;

    if (stream == stdout || stream == stderr)
        return EOF;

    if (stream->buf_pos >= stream->buf_end)
    {
        stream->eof = 1;
        return EOF;
    }

    int ch = (unsigned char)stream->buf[stream->buf_pos++];
    if (stream->buf_pos >= stream->buf_end)
        stream->eof = 1;

    return ch;
}

static void ensure_space_and_scroll(int *start_row, int start_col, int cur_len, int old_len)
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

int scanf(const char *fmt, ...)
{
    char *line = NULL;
    read_line(&line);
    if (!line)
        return 0;

    va_list args;
    va_start(args, fmt);
    int matched = 0;

    char *cursor = line;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p == 'd')
            {
                int val;
                int n = 0;
                if (sscanf(cursor, "%d%n", &val, &n) == 1)
                {
                    int *iptr = va_arg(args, int *);
                    *iptr = val;
                    matched++;
                    cursor += n;
                }
                else
                {
                    break;
                }
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                if (*cursor)
                {
                    *cptr = *cursor;
                    matched++;
                    cursor++;
                }
                else
                {
                    break;
                }
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                int n = 0;
                if (sscanf(cursor, "%s%n", sptr, &n) == 1)
                {
                    matched++;
                    cursor += n;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            if (*cursor != *p)
                break;
            cursor++;
        }

        while (*cursor == ' ' || *cursor == '\t')
            cursor++;
    }

    va_end(args);
    free(line);
    return matched;
}

int fscanf(FILE *stream, const char *fmt, ...)
{
    if (!stream)
        return 0;

    int file_size = fs_read_file_size(stream->name);
    if (file_size <= 0)
        return 0;

    char *buffer = (char *)stream->buf;
    int pos = 0;

    va_list args;
    va_start(args, fmt);
    int matched = 0;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p == 'd')
            {
                int sign = 1, val = 0;
                while (buffer[pos] == ' ' || buffer[pos] == '\n' || buffer[pos] == '\t')
                    pos++;
                if (buffer[pos] == '-')
                {
                    sign = -1;
                    pos++;
                }
                while (buffer[pos] >= '0' && buffer[pos] <= '9')
                {
                    val = val * 10 + (buffer[pos] - '0');
                    pos++;
                }
                int *iptr = va_arg(args, int *);
                *iptr = val * sign;
                matched++;
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                *cptr = buffer[pos++];
                matched++;
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                while (buffer[pos] == ' ' || buffer[pos] == '\n' || buffer[pos] == '\t')
                    pos++;
                while (buffer[pos] && buffer[pos] != ' ' && buffer[pos] != '\n' && buffer[pos] != '\t')
                {
                    *sptr++ = buffer[pos++];
                }
                *sptr = 0;
                matched++;
            }
        }
        else
        {
            if (buffer[pos++] != *p)
                break;
        }
    }

    va_end(args);
    return matched;
}

int sscanf(const char *str, const char *fmt, ...)
{
    int pos = 0;
    va_list args;
    va_start(args, fmt);
    int matched = 0;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p == 'd')
            {
                int sign = 1, val = 0;
                while (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\t')
                    pos++;
                if (str[pos] == '-')
                {
                    sign = -1;
                    pos++;
                }
                while (str[pos] >= '0' && str[pos] <= '9')
                {
                    val = val * 10 + (str[pos] - '0');
                    pos++;
                }
                int *iptr = va_arg(args, int *);
                *iptr = val * sign;
                matched++;
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                *cptr = str[pos++];
                matched++;
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                while (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\t')
                    pos++;
                while (str[pos] && str[pos] != ' ' && str[pos] != '\n' && str[pos] != '\t')
                {
                    *sptr++ = str[pos++];
                }
                *sptr = 0;
                matched++;
            }
        }
        else
        {
            if (str[pos++] != *p)
                break;
        }
    }

    va_end(args);
    return matched;
}

int fflush(FILE *stream)
{
    if (!stream)
        return EOF;

    if (stream == stdout || stream == stderr)
        return 0;

    if (stream->buf_pos == 0)
        return 0;

    int file_size_i = fs_read_file_size(stream->name);
    if (file_size_i < 0)
        file_size_i = 0;

    size_t file_size = (size_t)file_size_i;
    size_t need = file_size + (size_t)stream->buf_pos;

    if (need < file_size || need < (size_t)stream->buf_pos)
    {
        stream->err = 1;
        return EOF;
    }

    unsigned char *combined = (unsigned char *)malloc(need);
    if (!combined)
    {
        stream->err = 1;
        return EOF;
    }

    unsigned int got = 0;
    if (file_size > 0)
        if (fs_read_file(stream->name, combined, (unsigned int)file_size, &got) != 0)
            got = 0;

    memcpy(combined + got, stream->buf, (size_t)stream->buf_pos);

    int wres = fs_write_file(stream->name, combined, (unsigned int)(got + stream->buf_pos));
    free(combined);
    stream->buf_pos = 0;

    if (wres < 0)
    {
        stream->err = 1;
        return EOF;
    }

    return 0;
}

FILE *fopen(const char *pathname, const char *mode)
{
    if (!pathname || !mode)
        return NULL;

    FILE *f = alloc_file_slot();
    if (!f)
        return NULL;

    f->buf = NULL;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->name = strdup(pathname);
    if (!f->name)
    {
        free_file_slot(f);
        return NULL;
    }
    f->eof = 0;
    f->err = 0;

    if (mode[0] == 'r')
    {
        f->mode = 0;
        int size = fs_read_file_size(f->name);
        if (size >= 0)
        {
            if (size > 0)
            {
                f->buf = (unsigned char *)malloc(size);
                if (!f->buf)
                {
                    free_file_slot(f);
                    return NULL;
                }
                unsigned int got = 0;
                if (fs_read_file(f->name, f->buf, size, &got) != 0)
                {
                    free_file_slot(f);
                    return NULL;
                }
                f->buf_pos = 0;
                f->buf_end = got;
            }
            else
            {
                f->buf = NULL;
                f->buf_pos = 0;
                f->buf_end = 0;
            }
        }
        else
        {
            free_file_slot(f);
            return NULL;
        }
    }
    else if (mode[0] == 'w')
    {
        f->mode = 1;
        int del_rc = fs_delete_file(f->name);
        int rc = fs_make_file(f->name);
        if (rc != 0)
        {
            free_file_slot(f);
            return NULL;
        }
    }
    else if (mode[0] == 'a')
    {
        f->mode = 1;
        int size = fs_read_file_size(f->name);
        if (size > 0)
        {
            f->buf = (unsigned char *)malloc(size);
            if (!f->buf)
            {
                free_file_slot(f);
                return NULL;
            }
            unsigned int got = 0;
            if (fs_read_file(f->name, f->buf, size, &got) != 0)
            {
                free_file_slot(f);
                return NULL;
            }
            f->buf_pos = got;
            f->buf_end = got;
        }
        else
        {
            int rc = fs_make_file(f->name);
            if (rc != 0)
            {
                free_file_slot(f);
                return NULL;
            }
            f->buf_pos = 0;
            f->buf_end = 0;
        }
    }
    else
    {
        free_file_slot(f);
        return NULL;
    }

    return f;
}

int fclose(FILE *stream)
{
    if (!stream)
        return EOF;

    if (stream == stdout || stream == stderr || stream == stdin)
        return EOF;

    if (stream->mode == 1 && stream->buf_pos > 0)
        fflush(stream);

    free_file_slot(stream);
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0)
        return 0;
    size_t total = size * nmemb;
    size_t i;
    unsigned char *out = (unsigned char *)ptr;
    for (i = 0; i < total; i++)
    {
        int c = fgetc(stream);
        if (c == EOF)
            break;
        out[i] = (unsigned char)c;
    }
    return i / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0)
        return 0;
    size_t total = size * nmemb;
    const unsigned char *in = (const unsigned char *)ptr;
    size_t i;
    for (i = 0; i < total; i++)
        if (fputc(in[i], stream) == EOF)
            break;

    return i / size;
}

int fseek(FILE *stream, long offset, int whence)
{
    if (!stream)
        return -1;
    if (stream == stdout || stream == stderr)
        return -1;

    if (whence == SEEK_SET)
    {
        if (offset < 0)
            return -1;
        if ((unsigned long)offset <= stream->buf_end)
        {
            stream->buf_pos = (unsigned int)offset;
            stream->eof = (stream->buf_pos >= stream->buf_end);
            return 0;
        }
        if (stream->mode == 0)
        {
            int size = fs_read_file_size(stream->name);
            if (size >= 0)
            {
                stream->buf_end = (unsigned int)size;
                if ((unsigned long)offset <= stream->buf_end)
                {
                    stream->buf_pos = (unsigned int)offset;
                    stream->eof = (stream->buf_pos >= stream->buf_end);
                    return 0;
                }
            }
        }
        return -1;
    }
    else if (whence == SEEK_CUR)
    {
        long newpos = (long)stream->buf_pos + offset;
        return fseek(stream, newpos, SEEK_SET);
    }
    else if (whence == SEEK_END)
    {
        long newpos = (long)stream->buf_end + offset;
        return fseek(stream, newpos, SEEK_SET);
    }
    return -1;
}

long ftell(FILE *stream)
{
    if (!stream)
        return -1;
    return (long)stream->buf_pos;
}

void rewind(FILE *stream)
{
    if (!stream)
        return;
    fseek(stream, 0, SEEK_SET);
    clearerr(stream);
}

int remove(const char *pathname)
{
    if (!pathname)
        return -1;
    int res = fs_delete_file(pathname);
    return (res == 0) ? 0 : -1;
}

int rename(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath)
        return -1;

    FILE *oldf = fopen(oldpath, "r");
    if (!oldf)
        return -1;

    FILE *newf = fopen(newpath, "w");
    if (!newf)
    {
        fclose(oldf);
        return -1;
    }

    int ch;
    int write_err = 0;
    while ((ch = fgetc(oldf)) != EOF)
        if (fputc(ch, newf) == EOF)
        {
            write_err = 1;
            break;
        }

    if (!write_err && fflush(newf) == EOF)
        write_err = 1;

    int cerr_new = fclose(newf);
    int cerr_old = fclose(oldf);

    if (write_err || cerr_new != 0 || cerr_old != 0)
    {
        remove(newpath);
        return -1;
    }

    int del = fs_delete_file(oldpath);
    if (del != 0)
        return -1;

    return 0;
}

int feof(FILE *stream)
{
    if (!stream)
        return 0;
    return stream->eof;
}

int ferror(FILE *stream)
{
    if (!stream)
        return 0;
    return stream->err;
}

void clearerr(FILE *stream)
{
    if (!stream)
        return;
    stream->err = 0;
    stream->eof = 0;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    (void)stream;
    (void)buf;
    (void)mode;
    (void)size;
    return 0;
}

void setbuf(FILE *stream, char *buf)
{
    (void)stream;
    (void)buf;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    char tmp[64];
    size_t written = 0;
    size_t outpos = 0;

#define PUTCHAR_TO_BUF(ch)                     \
    do                                         \
    {                                          \
        if (outpos + 1 < size)                 \
        {                                      \
            buf[outpos] = (char)(ch);          \
        }                                      \
        outpos += (outpos + 1 < size) ? 1 : 0; \
        written++;                             \
    } while (0)

#define PUTN_TO_BUF(ptr, n)                            \
    do                                                 \
    {                                                  \
        for (size_t __i = 0; __i < (size_t)(n); ++__i) \
        {                                              \
            if (outpos + 1 < size)                     \
                buf[outpos] = (ptr)[__i];              \
            outpos += (outpos + 1 < size) ? 1 : 0;     \
            written++;                                 \
        }                                              \
    } while (0)

    if (!buf && size != 0)
        return -1;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%' && *(p + 1))
        {
            p++;
            int long_flag = 0;
            if (*p == 'l')
            {
                long_flag = 1;
                p++;
            }

            if (*p == 's')
            {
                const char *s = va_arg(args, const char *);
                if (!s)
                    s = "(null)";
                while (*s)
                {
                    PUTCHAR_TO_BUF(*s++);
                }
            }
            else if (*p == 'c')
            {
                char ch = (char)va_arg(args, int);
                PUTCHAR_TO_BUF(ch);
            }
            else if (*p == 'd')
            {
                long val = long_flag ? va_arg(args, long) : va_arg(args, int);
                int neg = 0;
                int i = 0;
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                if (val < 0)
                {
                    neg = 1;
                    val = -val;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                if (neg)
                    PUTCHAR_TO_BUF('-');
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'u')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'x' || *p == 'X')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                const char *digits = (*p == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'p')
            {
                void *ptr = va_arg(args, void *);
                unsigned long val = (unsigned long)ptr;
                PUTN_TO_BUF("0x", 2);
                int i = 0;
                const char *digits = "0123456789abcdef";
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'f' || *p == 'g')
            {
                double val = va_arg(args, double);
                if (val < 0)
                {
                    PUTCHAR_TO_BUF('-');
                    val = -val;
                }

                long int_part = (long)val;
                double frac_part = val - int_part;
                int i = 0;
                if (int_part == 0)
                {
                    tmp[i++] = '0';
                }
                else
                {
                    long tmpval = int_part;
                    char tmpbuf[32];
                    int j = 0;
                    while (tmpval > 0 && j < (int)sizeof(tmpbuf))
                    {
                        tmpbuf[j++] = '0' + tmpval % 10;
                        tmpval /= 10;
                    }
                    while (j-- > 0)
                        tmp[i++] = tmpbuf[j];
                }
                tmp[i] = 0;
                PUTN_TO_BUF(tmp, i);

                PUTCHAR_TO_BUF('.');

                for (int d = 0; d < 10; d++)
                {
                    frac_part *= 10.0;
                    int digit = (int)frac_part;
                    PUTCHAR_TO_BUF((char)('0' + digit));
                    frac_part -= digit;
                }
            }
            else if (*p == '%')
            {
                PUTCHAR_TO_BUF('%');
            }
            else
            {
                PUTCHAR_TO_BUF('%');
                PUTCHAR_TO_BUF(*p);
            }
        }
        else
        {
            PUTCHAR_TO_BUF(*p);
        }
    }

    if (size > 0)
    {
        size_t nulpos = (outpos < size) ? outpos : (size - 1);
        buf[nulpos] = '\0';
    }

#undef PUTCHAR_TO_BUF
#undef PUTN_TO_BUF

    return (int)written;
}

int vsprintf(char *str, const char *format, va_list ap)
{
    return vsnprintf(str, (size_t)-1, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsnprintf(str, size, format, ap);
    va_end(ap);
    return r;
}

int sprintf(char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsnprintf(str, (size_t)-1, format, ap);
    va_end(ap);
    return r;
}