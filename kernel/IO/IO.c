#include "IO.h"

volatile unsigned short *video = (volatile unsigned short *)0xB8000;
int row = 0, col = 0;

// Draw a single character at given row/col
void draw_char_at(int r, int c, char ch)
{
    video[r * 80 + c] = (0x07 << 8) | ch;
}

// Redraw a buffer on the current row after prompt
void redraw_buffer(char *buffer, int len, int prompt_len)
{
    for (int i = 0; i < len; i++)
        draw_char_at(row, prompt_len + i, buffer[i]);
    draw_char_at(row, prompt_len + len, ' '); // clear extra char
}

// VGA cursor update
void update_cursor()
{
    unsigned short pos = row * 80 + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

// Scroll screen up if needed
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

// Clear screen
void clear(void)
{
    for (int i = 0; i < 80 * 25; i++)
        video[i] = (0x07 << 8) | ' ';
    row = 0;
    col = 0;
    update_cursor();
}

// Write string to screen
void write(const char *str)
{
    write_colored(str, 0x07);
}

// Write string to the screen using a specific color
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

// Write a single character to screen at current cursor
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

// Read keyboard input into buffer (blocking) with prompt offset and caret support
void read(char *buffer, int max_len, int prompt_len)
{
    unsigned char scancode;
    char c;
    unsigned char status;
    unsigned char shift = 0;
    int buf_idx = 0;   // number of chars in buffer
    int caret_pos = 0; // caret position in line

    col = prompt_len;
    update_cursor();

    while (1)
    {
        // Poll keyboard
        do
        {
            __asm__ volatile("inb $0x64, %0" : "=a"(status));
        } while (!(status & 0x01));
        __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

        // Shift press/release
        if (scancode == 0x2A || scancode == 0x36)
        {
            shift = 1;
            continue;
        }
        if (scancode == 0xAA || scancode == 0xB6)
        {
            shift = 0;
            continue;
        }
        if (scancode & 0x80)
            continue; // ignore key releases

        // Arrow keys
        if (scancode == 0x4B && caret_pos > 0)
        {
            caret_pos--;
            col--;
            update_cursor();
            continue;
        } // left
        if (scancode == 0x4D && caret_pos < buf_idx)
        {
            caret_pos++;
            col++;
            update_cursor();
            continue;
        } // right

        // Backspace
        if (scancode == 0x0E && caret_pos > 0)
        {
            for (int i = caret_pos - 1; i < buf_idx - 1; i++)
                buffer[i] = buffer[i + 1];
            buf_idx--;
            caret_pos--;
            redraw_buffer(buffer, buf_idx, prompt_len);
            col = prompt_len + caret_pos;
            update_cursor();
            continue;
        }

        // Enter
        if (scancode == 0x1C)
        {
            buffer[buf_idx] = '\0';
            row++;
            col = 0;
            scroll();
            update_cursor();
            break;
        }

        // Scancode to ASCII
        if (scancode >= 0x02 && scancode <= 0x0B)
        {
            const char u[] = "1234567890";
            const char s[] = "!@#$%^&*()";
            c = shift ? s[scancode - 0x02] : u[scancode - 0x02];
        }
        else if (scancode >= 0x10 && scancode <= 0x19)
        {
            const char *u = "qwertyuiop";
            const char *s = "QWERTYUIOP";
            c = shift ? s[scancode - 0x10] : u[scancode - 0x10];
        }
        else if (scancode >= 0x1E && scancode <= 0x26)
        {
            const char *u = "asdfghjkl";
            const char *s = "ASDFGHJKL";
            c = shift ? s[scancode - 0x1E] : u[scancode - 0x1E];
        }
        else if (scancode >= 0x2C && scancode <= 0x32)
        {
            const char *u = "zxcvbnm";
            const char *s = "ZXCVBNM";
            c = shift ? s[scancode - 0x2C] : u[scancode - 0x2C];
        }
        else if (scancode == 0x27)
            c = shift ? '?' : ';';
        else if (scancode == 0x33)
            c = shift ? '>' : '.';
        else if (scancode == 0x34)
            c = shift ? '<' : ',';
        else if (scancode == 0x0D)
            c = shift ? '+' : '=';
        else if (scancode == 0x0C)
            c = shift ? '_' : '-';
        else if (scancode == 0x35)
            c = shift ? '\\' : '/';
        else if (scancode == 0x39)
            c = ' ';
        else
            continue;

        // Insert character at caret position
        if (buf_idx < max_len - 1)
        {
            for (int i = buf_idx; i > caret_pos; i--)
                buffer[i] = buffer[i - 1];
            buffer[caret_pos] = c;
            buf_idx++;
            caret_pos++;
            redraw_buffer(buffer, buf_idx, prompt_len);
            col = prompt_len + caret_pos;
            update_cursor();
        }
    }
}
