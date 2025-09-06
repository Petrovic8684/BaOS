#include "display.h"
#include "../../helpers/ports/ports.h"

static volatile unsigned short *video = (volatile unsigned short *)0xB8000;
static int row = 0, col = 0;
static unsigned char cur_attr = 0x07;

enum ansi_state
{
    ANSI_NORMAL = 0,
    ANSI_ESC,
    ANSI_CSI
};

static const int ansi_to_vga[8] = {
    0,
    4,
    2,
    6,
    1,
    5,
    3,
    7};

static int ansi_state = ANSI_NORMAL;
static char ansi_params[64];
static int ansi_params_len = 0;

static void update_cursor(int new_row, int new_col)
{
    row = new_row;
    col = new_col;

    if (row < 0)
        row = 0;
    if (row > 24)
        row = 24;
    if (col < 0)
        col = 0;
    if (col > 79)
        col = 79;

    unsigned short pos = row * 80 + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

static void scroll(void)
{
    if (row < 25)
        return;

    for (int r = 1; r < 25; r++)
        for (int c = 0; c < 80; c++)
            video[(r - 1) * 80 + c] = video[r * 80 + c];

    for (int c = 0; c < 80; c++)
        video[24 * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';

    row = 24;
    update_cursor(row, col);
}

void clear(void)
{
    for (int i = 0; i < 80 * 25; i++)
        video[i] = ((unsigned short)cur_attr << 8) | ' ';

    row = 0;
    col = 0;

    update_cursor(row, col);
}

static void draw_char_at(int r, int c, char ch)
{
    if (r < 0)
        r = 0;
    if (r >= 25)
        r = 24;
    if (c < 0)
        c = 0;
    if (c >= 80)
        c = 79;
    video[r * 80 + c] = ((unsigned short)cur_attr << 8) | (unsigned char)ch;
}

static void vga_put_char(char ch)
{
    if (ch == '\n')
    {
        col = 0;
        row++;
        scroll();
        return;
    }
    else if (ch == '\r')
    {
        col = 0;
        update_cursor(row, col);
        return;
    }
    else if (ch == '\t')
    {
        int spaces = 4 - (col % 4);
        for (int i = 0; i < spaces; i++)
        {
            draw_char_at(row, col, ' ');
            col++;
            if (col >= 80)
            {
                col = 0;
                row++;
                scroll();
            }
        }
        update_cursor(row, col);
        return;
    }
    else if (ch == '\b')
    {
        if (col > 0)
            col--;
        else if (row > 0)
        {
            row--;
            col = 79;
        }
        draw_char_at(row, col, ' ');
        update_cursor(row, col);
        return;
    }

    draw_char_at(row, col, ch);
    col++;
    if (col >= 80)
    {
        col = 0;
        row++;
        scroll();
    }
    update_cursor(row, col);
}

static int parse_params(const char *s, int *out, int max)
{
    int count = 0;
    const char *p = s;
    while (*p && count < max)
    {
        while (*p == ';')
            p++;
        if (!*p)
            break;
        int val = 0;
        int seen = 0;
        while (*p >= '0' && *p <= '9')
        {
            seen = 1;
            val = val * 10 + (*p - '0');
            p++;
        }
        if (!seen)
            out[count++] = -1;
        else
            out[count++] = val;
        if (*p == ';')
            p++;
    }
    return count;
}

static void handle_csi(const char *params, char final)
{
    int pvals[8];
    for (int i = 0; i < 8; i++)
        pvals[i] = -1;
    int pcount = 0;
    if (params && *params)
        pcount = parse_params(params, pvals, 8);

    switch (final)
    {
    case 'A':
    {
        int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
        row -= n;
        if (row < 0)
            row = 0;
        update_cursor(row, col);
        break;
    }
    case 'B':
    {
        int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
        row += n;
        if (row > 24)
            row = 24;
        update_cursor(row, col);
        break;
    }
    case 'C':
    {
        int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
        col += n;
        if (col > 79)
            col = 79;
        update_cursor(row, col);
        break;
    }
    case 'D':
    {
        int n = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
        col -= n;
        if (col < 0)
            col = 0;
        update_cursor(row, col);
        break;
    }
    case 'H':
    case 'f':
    {
        int r = (pcount >= 1 && pvals[0] > 0) ? pvals[0] : 1;
        int c = (pcount >= 2 && pvals[1] > 0) ? pvals[1] : 1;
        row = r - 1;
        col = c - 1;
        if (row < 0)
            row = 0;
        if (row > 24)
            row = 24;
        if (col < 0)
            col = 0;
        if (col > 79)
            col = 79;
        update_cursor(row, col);
        break;
    }
    case 'J':
    {
        int mode = (pcount >= 1 && pvals[0] >= 0) ? pvals[0] : 0;
        if (mode == 2)
            clear();
        else if (mode == 0)
        {
            for (int c = col; c < 80; c++)
                video[row * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
            for (int r = row + 1; r < 25; r++)
                for (int c = 0; c < 80; c++)
                    video[r * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
            update_cursor(row, col);
        }
        else if (mode == 1)
        {
            for (int c = 0; c <= col; c++)
                video[row * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
            for (int r = 0; r < row; r++)
                for (int c = 0; c < 80; c++)
                    video[r * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
            update_cursor(row, col);
        }
        break;
    }
    case 'K':
    {
        int mode = (pcount >= 1 && pvals[0] >= 0) ? pvals[0] : 0;
        if (mode == 0)
            for (int c = col; c < 80; c++)
                video[row * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
        else if (mode == 1)
            for (int c = 0; c <= col; c++)
                video[row * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
        else if (mode == 2)
            for (int c = 0; c < 80; c++)
                video[row * 80 + c] = ((unsigned short)cur_attr << 8) | ' ';
        update_cursor(row, col);
        break;
    }
    case 'm':
    {
        if (pcount == 0)
        {
            cur_attr = 0x07;
            break;
        }
        for (int i = 0; i < pcount; i++)
        {
            int p = pvals[i];
            if (p == -1)
                p = 0;

            if (p == 0)
            {
                cur_attr = 0x07;
            }
            else if (p == 1)
            {
                cur_attr |= 0x08;
            }
            else if (p >= 30 && p <= 37)
            {
                unsigned char intensity = cur_attr & 0x08;
                int fg = ansi_to_vga[p - 30] & 0x07;
                unsigned char bg_nibble = cur_attr & 0xF0;
                cur_attr = bg_nibble | (fg | intensity);
            }
            else if (p >= 40 && p <= 47)
            {
                int bg = ansi_to_vga[p - 40] & 0x07;
                unsigned char fg_nibble = cur_attr & 0x0F;
                cur_attr = (unsigned char)((bg << 4) | fg_nibble);
            }
            else if (p >= 90 && p <= 97)
            {
                int fg = (ansi_to_vga[p - 90] & 0x07) | 0x08;
                unsigned char bg_nibble = cur_attr & 0xF0;
                cur_attr = bg_nibble | (fg & 0x0F);
            }
            else if (p >= 100 && p <= 107)
            {
                int bg = (ansi_to_vga[p - 100] & 0x07) | 0x08;
                unsigned char fg_nibble = cur_attr & 0x0F;
                cur_attr = (unsigned char)(((bg & 0x0F) << 4) | fg_nibble);
            }
            else if (p == 39)
            {
                unsigned char intensity = cur_attr & 0x08;
                unsigned char bg_nibble = cur_attr & 0xF0;
                cur_attr = bg_nibble | (0x07 | intensity);
            }
            else if (p == 49)
            {
                unsigned char fg_nibble = cur_attr & 0x0F;
                cur_attr = (unsigned char)((0 << 4) | fg_nibble);
            }
        }
        break;
    }

    default:
        break;
    }
}

void write(const char *str)
{
    if (!str)
        return;

    for (const unsigned char *p = (const unsigned char *)str; *p; p++)
    {
        unsigned char ch = *p;
        switch (ansi_state)
        {
        case ANSI_NORMAL:
            if (ch == 0x1B)
            {
                ansi_state = ANSI_ESC;
                ansi_params_len = 0;
                ansi_params[0] = '\0';
            }
            else
            {
                vga_put_char((char)ch);
            }
            break;
        case ANSI_ESC:
            if (ch == '[')
            {
                ansi_state = ANSI_CSI;
                ansi_params_len = 0;
                ansi_params[0] = '\0';
            }
            else
            {
                ansi_state = ANSI_NORMAL;
                vga_put_char((char)ch);
            }
            break;
        case ANSI_CSI:
            if (ch >= 0x40 && ch <= 0x7E)
            {
                ansi_params[ansi_params_len] = '\0';
                handle_csi(ansi_params, (char)ch);
                ansi_state = ANSI_NORMAL;
                ansi_params_len = 0;
            }
            else
            {
                if (ansi_params_len < (int)sizeof(ansi_params) - 1)
                    ansi_params[ansi_params_len++] = (char)ch;
                else
                {
                    ansi_state = ANSI_NORMAL;
                    ansi_params_len = 0;
                }
            }
            break;
        }
    }
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

void write_char(char ch)
{
    vga_put_char(ch);
}

unsigned int get_cursor_row()
{
    return row;
}

unsigned int get_cursor_col()
{
    return col;
}