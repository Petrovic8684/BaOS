#include <stdio.h>
#include <string.h>

#define SYS_WRITE 1
#define SYS_GET_CURSOR_ROW 4
#define SYS_GET_CURSOR_COL 5
#define SYS_DRAW_CHAR_AT 9
#define SYS_READ 10
#define SYS_UPDATE_CURSOR 25
#define SYS_FS_MAKE_FILE 14
#define SYS_FS_DELETE_FILE 18
#define SYS_FS_WRITE_FILE 19
#define SYS_FS_READ_FILE 20

#define MAX_OPEN_FILES 16

static inline void write(const char *str)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline int get_cursor_row(void)
{
    int row;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(row)
        : [num] "i"(SYS_GET_CURSOR_ROW)
        : "eax", "ebx", "memory");
    return row;
}

static inline int get_cursor_col(void)
{
    int col;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(col)
        : [num] "i"(SYS_GET_CURSOR_COL)
        : "eax", "ebx", "memory");
    return col;
}

static inline void draw_char_at(int r, int c, char ch)
{
    struct
    {
        int r;
        int c;
        char ch;
    } args = {r, c, ch};

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_DRAW_CHAR_AT), [a] "r"(&args)
        : "eax", "ebx", "memory");
}

static inline void update_cursor(int row, int col)
{
    struct
    {
        int row;
        int col;
    } args = {row, col};

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_UPDATE_CURSOR), [a] "r"(&args)
        : "eax", "ebx", "memory");
}

static inline char read(void)
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

static inline int fs_read_file(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_READ_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
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
        fs_write_file(stream->name, (char *)stream->buf);
        stream->buf_pos = 0;
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
    return (int)read();
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
        int size = fs_read_file(stream->name);
        if (size <= 0)
        {
            stream->eof = 1;
            return EOF;
        }
        stream->buf_end = (unsigned int)size;
        stream->buf_pos = 0;
    }

    int ch = (int)stream->buf[stream->buf_pos++];
    if (stream->buf_pos >= stream->buf_end)
        stream->eof = 1;
    return ch;
}

static void redraw_buffer(const char *buf, int len, int start_row, int start_col)
{
    for (int i = 0; i < len; i++)
        draw_char_at(start_row, start_col + i, buf[i]);

    draw_char_at(start_row, start_col + len, ' ');

    update_cursor(start_row, start_col + len);
}

void read_line(char *buf, int max_len)
{
    int len = 0;
    int cursor = 0;
    int row = get_cursor_row();
    int col = get_cursor_col();

    while (1)
    {
        char c = read();

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

    int file_size = fs_read_file(stream->name);
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

    int res = fs_write_file(stream->name, (char *)stream->buf);
    stream->buf_pos = 0;

    if (res < 0)
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

    f->name = pathname;
    f->pos = 0;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->eof = 0;
    f->err = 0;

    if (mode[0] == 'r')
    {
        f->mode = 0;
        int size = fs_read_file(f->name);
        if (size > 0)
        {
            f->buf_end = (unsigned int)size;
            f->buf_pos = 0;
        }
        else
        {
            f->buf_end = 0;
            f->buf_pos = 0;
        }
    }
    else if (mode[0] == 'w')
    {
        f->mode = 1;
        fs_make_file(f->name);
        f->buf_pos = 0;
        f->buf_end = 0;
    }
    else if (mode[0] == 'a')
    {
        f->mode = 1;
        int size = fs_read_file(f->name);
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
            int size = fs_read_file(stream->name);
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