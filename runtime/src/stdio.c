#include <stdio.h>
#include <stdarg.h>

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
    if (stream->buf_pos >= BUFSIZE)
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

    if (stream->buf_pos == 0)
    {
        int size = fs_read_file(stream->name);
        if (size <= 0)
            return EOF;
    }

    return (int)stream->buf[stream->buf_pos++];
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

    fs_write_file(stream->name, (char *)stream->buf);
    stream->buf_pos = 0;

    return 0;
}