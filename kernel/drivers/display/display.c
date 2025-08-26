#include "display.h"

volatile unsigned short *video = (volatile unsigned short *)0xB8000;
int row = 0, col = 0;

void draw_char_at(int r, int c, char ch)
{
    video[r * 80 + c] = (0x07 << 8) | ch;
}

void update_cursor()
{
    unsigned short pos = row * 80 + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void scroll(void)
{
    if (row < 25)
        return;

    for (int r = 1; r < 25; r++)
        for (int c = 0; c < 80; c++)
            video[(r - 1) * 80 + c] = video[r * 80 + c];

    for (int c = 0; c < 80; c++)
        video[24 * 80 + c] = (0x07 << 8) | ' ';

    row = 24;
    update_cursor();
}

void clear(void)
{
    for (int i = 0; i < 80 * 25; i++)
        video[i] = (0x07 << 8) | ' ';

    row = 0;
    col = 0;

    update_cursor();
}

void write(const char *str)
{
    write_colored(str, 0x07);
}

void write_colored(const char *str, unsigned char color)
{
    while (*str)
    {
        if (*str == '\n')
        {
            col = 0;
            row++;

            scroll();
        }
        else
        {
            video[row * 80 + col] = (color << 8) | *str;

            col++;
            if (col >= 80)
            {
                col = 0;
                row++;

                scroll();
            }
        }

        str++;
        update_cursor();
    }
}

void write_char(char ch)
{
    if (ch == '\n')
    {
        col = 0;
        row++;

        scroll();
    }
    else
    {
        draw_char_at(row, col, ch);

        col++;
        if (col >= 80)
        {
            col = 0;
            row++;

            scroll();
        }
    }

    update_cursor();
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

void redraw_buffer(char *buffer, int len, int prompt_len)
{
    for (int i = 0; i < len; i++)
        draw_char_at(row, prompt_len + i, buffer[i]);

    draw_char_at(row, prompt_len + len, ' ');
}

void redraw_and_clear(const char *buffer, int len, int prompt_len, int prev_len)
{
    int i;

    for (i = 0; i < len; i++)
        draw_char_at(row, prompt_len + i, buffer[i]);

    for (i = len; i < prev_len; i++)
        draw_char_at(row, prompt_len + i, ' ');

    draw_char_at(row, prompt_len + len, ' ');
}