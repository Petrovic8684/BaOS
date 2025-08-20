volatile unsigned short *video = (volatile unsigned short *)0xB8000;
int row = 0, col = 0;

// Simple outb function
static inline void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// -----------------
// VGA cursor update
// -----------------
void update_cursor()
{
    unsigned short pos = row * 80 + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

// -----------------
// Scroll screen up if needed
// -----------------
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

// -----------------
// Clear screen
// -----------------
void clear(void)
{
    for (int i = 0; i < 80 * 25; i++)
        video[i] = (0x07 << 8) | ' ';
    row = 0;
    col = 0;
    update_cursor();
}

// -----------------
// Write string to screen
// -----------------
void write(const char *str)
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
            video[row * 80 + col] = (0x07 << 8) | *str;
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

// -----------------
// Read keyboard input into buffer (blocking)
// -----------------
void read(char *buffer, int max_len)
{
    unsigned char scancode;
    char c;
    unsigned char status;
    unsigned char shift = 0;
    int buf_idx = 0;

    while (1)
    {
        do
        {
            __asm__ volatile("inb $0x64, %0" : "=a"(status));
        } while (!(status & 0x01));
        __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

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
            continue;

        // Backspace
        if (scancode == 0x0E && buf_idx > 0)
        {
            buf_idx--;
            if (col > 0)
                col--;
            video[row * 80 + col] = (0x07 << 8) | ' ';
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

        if (buf_idx < max_len - 1)
        {
            buffer[buf_idx++] = c;
            video[row * 80 + col++] = (0x07 << 8) | c;
            if (col >= 80)
            {
                col = 0;
                row++;
                scroll();
            }
            update_cursor();
        }
    }
}
