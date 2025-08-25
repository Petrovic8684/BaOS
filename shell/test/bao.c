void print_char(char c, unsigned char color, unsigned char row, unsigned char col)
{
    unsigned short *video = (unsigned short *)0xB8000;
    video[row * 80 + col] = ((unsigned short)color << 8) | c;
}

void print_string(const char *str, unsigned char color, unsigned char row, unsigned char col)
{
    unsigned char x = col;
    unsigned char y = row;
    while (*str)
    {
        if (*str == '\n')
        {
            y++;
            x = col;
        }
        else
        {
            print_char(*str, color, y, x);
            x++;
        }
        str++;
    }
}

void _start()
{
    print_string("User program works!", 0x0F, 5, 10);

    return;
}
