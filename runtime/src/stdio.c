#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_FS_MAKE_FILE 13
#define SYS_FS_DELETE_FILE 14
#define SYS_FS_WRITE_FILE 15
#define SYS_FS_READ_FILE 16
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19

#define MAX_OPEN_FILES 16
#define USER_BUFFER_SIZE 1024

static inline void sys_write(const char *str)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline char sys_read(void)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_READ)
        : "eax", "ebx", "memory");
    return (char)(ret & 0xFF);
}

static inline int fs_make_file(const char *name)
{
    unsigned int ret;
    asm volatile(
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
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_write_file(const char *name, const char *text)
{
    struct
    {
        const char *name;
        const char *text;
    } args = {name, text};
    unsigned int ret;
    asm volatile(
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
    asm volatile(
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
    asm volatile(
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
    asm volatile(
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
    sys_write(seq);
}

static void update_cursor(int new_row, int new_col)
{
    if (new_row < 0)
        new_row = 0;
    if (new_row > 24)
        new_row = 24;
    if (new_col < 0)
        new_col = 0;
    if (new_col > 79)
        new_col = 79;

    tty_row = new_row;
    tty_col = new_col;
    send_move_syscall(tty_row + 1, tty_col + 1);
}

static int get_cursor_row(void) { return tty_row; }
static int get_cursor_col(void) { return tty_col; }

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
                if (tty_row > 24)
                    tty_row = 24;
            }
            else if (ch == '\r')
            {
                tty_col = 0;
            }
            else if (ch == '\t')
            {
                int spaces = 4 - (tty_col % 4);
                tty_col += spaces;
                if (tty_col > 79)
                {
                    tty_col = 0;
                    tty_row++;
                    if (tty_row > 24)
                        tty_row = 24;
                }
            }
            else if (ch == '\b')
            {
                if (tty_col > 0)
                    tty_col--;
                else if (tty_row > 0)
                {
                    tty_row--;
                    tty_col = 79;
                }
            }
            else
            {
                tty_col++;
                if (tty_col >= 80)
                {
                    tty_col = 0;
                    tty_row++;
                    if (tty_row > 24)
                        tty_row = 24;
                }
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
                    if (!seen)
                        pvals[pcount++] = -1;
                    else
                        pvals[pcount++] = val;
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
                    if (tty_row > 24)
                        tty_row = 24;
                    break;
                }
                case 'C':
                {
                    int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
                    tty_col += n;
                    if (tty_col > 79)
                        tty_col = 79;
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
                    if (tty_row > 24)
                        tty_row = 24;
                    if (tty_col < 0)
                        tty_col = 0;
                    if (tty_col > 79)
                        tty_col = 79;
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
                {
                    break;
                }
                case 'm':
                {
                    if (pcount == 0)
                    {
                        tty_attr = 0x07;
                    }
                    else
                    {
                        for (int i = 0; i < pcount; i++)
                        {
                            int p = pvals[i];
                            if (p == -1)
                                p = 0;
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
                default:
                    break;
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
}

void write_char(char ch)
{
    char tmp[2] = {ch, 0};
    write(tmp);
}

void write_dec(unsigned int v)
{
    char buf[12];
    int i = 0;
    if (v == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        char rev[12];
        int j = 0;
        while (v > 0 && j < (int)sizeof(rev))
        {
            rev[j++] = '0' + (v % 10);
            v /= 10;
        }
        while (j-- > 0)
            buf[i++] = rev[j];
    }
    buf[i] = '\0';
    write(buf);
}

void write_hex(unsigned int val)
{
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++)
    {
        unsigned char nibble = (val >> ((7 - i) * 4)) & 0xF;
        buf[2 + i] = (nibble < 10) ? ('0' + nibble) : ('A' + (nibble - 10));
    }
    buf[10] = '\0';
    write(buf);
}

static FILE _file_table[MAX_OPEN_FILES];

static FILE *alloc_file_slot(void)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (_file_table[i].name == NULL)
        {
            _file_table[i].name = NULL;
            _file_table[i].mode = 0;
            _file_table[i].pos = 0;
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

    f->name = NULL;
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
    if (stream == stdout || stream == stderr)
    {
        char buf[2] = {(char)c, 0};
        write(buf);
        return c;
    }

    if (!stream)
        return EOF;

    stream->buf[stream->buf_pos++] = (unsigned char)c;

    if (stream->buf_pos >= BUFSIZ)
    {
        unsigned char tmp_all[USER_BUFFER_SIZE];
        unsigned int got = 0;

        int rc = fs_read_file(stream->name, tmp_all, USER_BUFFER_SIZE, &got);
        if (rc != 0)
            got = 0;

        unsigned int can_add = USER_BUFFER_SIZE - got - 1;
        if (can_add > (unsigned int)stream->buf_pos)
            can_add = (unsigned int)stream->buf_pos;

        if (can_add > 0)
            memcpy(tmp_all + got, stream->buf, can_add);

        tmp_all[got + can_add] = '\0';

        int wres = fs_write_file(stream->name, (const char *)tmp_all);
        stream->buf_pos = 0;

        if (wres < 0)
        {
            stream->err = 1;
            return EOF;
        }
    }

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
                int val = va_arg(args, int);
                int neg = 0;
                int i = 0;
                if (val == 0)
                {
                    if (fputc('0', stream) == EOF)
                        return -1;
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
                    if (fputc('-', stream) == EOF)
                        return -1;
                    total++;
                }
                while (i-- > 0)
                {
                    if (fputc(tmp[i], stream) == EOF)
                        return -1;
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

                if (*p == 'g')
                {
                    int end = total - 1;
                    while (tmp[end] == '0')
                        end--;
                }
            }
            else if (*p == '%')
            {
                if (fputc('%', stream) == EOF)
                    return -1;
                total++;
            }
            else
            {
                if (fputc('%', stream) == EOF)
                    return -1;
                if (fputc(*p, stream) == EOF)
                    return -1;
                total += 2;
            }
        }
        else
        {
            if (fputc(*p, stream) == EOF)
                return -1;
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

static void redraw_buffer(const char *buf, int len, int start_row, int start_col)
{
    send_move_syscall(start_row + 1, start_col + 1);

    if (len > 0)
    {
        char tmp[BUFSIZ + 1];
        if (len > BUFSIZ)
            len = BUFSIZ;
        memcpy(tmp, buf, len);
        tmp[len] = '\0';
        write(tmp);
    }

    send_move_syscall(start_row + 1, start_col + len + 1);
    sys_write(" ");
    update_cursor(start_row, start_col + len);
}

void read_line(char *buf, int max_len)
{
    int len = 0;
    int cursor = 0;

    tty_row = sys_get_cursor_row();
    tty_col = sys_get_cursor_col();

    int row = tty_row;
    int col = tty_col;

    while (1)
    {
        char c = sys_read();

        if (c == '\n' || c == '\r')
        {
            buf[len] = 0;
            update_cursor(row, col + len);
            write("\n");
            break;
        }

        if (c == 27)
        {
            buf[len] = 27;
            update_cursor(row, col + len);
            write("\n");
            break;
        }

        if (c == 1 || c == 2)
        {
            buf[0] = c;
            buf[1] = 0;
        }

        if (c == 3)
        {
            if (cursor > 0)
                cursor--;
            update_cursor(row, col + cursor);
            continue;
        }
        if (c == 4)
        {
            if (cursor < len)
                cursor++;
            update_cursor(row, col + cursor);
            continue;
        }

        if (c == 8 || c == 127)
        {
            if (cursor > 0)
            {
                for (int j = cursor - 1; j < len - 1; j++)
                    buf[j] = buf[j + 1];
                len--;
                cursor--;

                buf[len] = 0;
                redraw_buffer(buf, len, row, col);
                update_cursor(row, col + cursor);
            }
            continue;
        }

        if ((unsigned char)c < 32)
            continue;

        if (len < max_len - 1)
        {
            for (int j = len; j > cursor; j--)
                buf[j] = buf[j - 1];

            buf[cursor] = c;
            len++;
            cursor++;

            buf[len] = 0;
            redraw_buffer(buf, len, row, col);
            update_cursor(row, col + cursor);
        }
    }
}

int scanf(const char *fmt, ...)
{
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
                char c;
                do
                {
                    c = getchar();
                } while (c == ' ' || c == '\n' || c == '\t');
                if (c == '-')
                {
                    sign = -1;
                    c = getchar();
                }
                while (c >= '0' && c <= '9')
                {
                    val = val * 10 + (c - '0');
                    c = getchar();
                }
                int *iptr = va_arg(args, int *);
                *iptr = val * sign;
                matched++;
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                *cptr = getchar();
                matched++;
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                read_line(sptr, 512);
                matched++;
            }
        }
        else
        {
            char c = getchar();
            if (c != *p)
                break;
        }
    }

    va_end(args);
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

    {
        unsigned char combined[USER_BUFFER_SIZE];
        unsigned int got = 0;

        int rc = fs_read_file(stream->name, combined, USER_BUFFER_SIZE, &got);
        if (rc != 0)
            got = 0;

        unsigned int can_add = USER_BUFFER_SIZE - got - 1;
        if (can_add > (unsigned int)stream->buf_pos)
            can_add = (unsigned int)stream->buf_pos;

        if (can_add > 0)
            memcpy(combined + got, stream->buf, can_add);

        combined[got + can_add] = '\0';

        int wres = fs_write_file(stream->name, (const char *)combined);
        stream->buf_pos = 0;
        if (wres < 0)
        {
            stream->err = 1;
            return EOF;
        }
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

    f->name = pathname;
    f->pos = 0;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->eof = 0;
    f->err = 0;

    if (mode[0] == 'r')
    {
        f->mode = 0;
        unsigned int got = 0;
        int rc = fs_read_file(f->name, f->buf, BUFSIZ, &got);
        if (rc == 0)
        {
            f->buf_end = got;
            f->buf_pos = 0;
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

        fs_delete_file(f->name);
        fs_make_file(f->name);

        f->buf_pos = 0;
        f->buf_end = 0;
    }
    else if (mode[0] == 'a')
    {
        f->mode = 1;
        int size = fs_read_file_size(f->name);
        if (size > 0)
        {
            f->buf_end = (unsigned int)size;
            f->buf_pos = f->buf_end % BUFSIZ;
        }
        else
        {
            fs_make_file(f->name);
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
        if (fflush(stream) == EOF)
            stream->err = 1;

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
    while ((ch = fgetc(oldf)) != EOF)
        if (fputc(ch, newf) == EOF)
        {
            fclose(oldf);
            fclose(newf);
            return -1;
        }

    fflush(newf);
    fclose(oldf);
    fclose(newf);

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
