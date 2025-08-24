#include "keyboard.h"

char read(void)
{
    unsigned char scancode;
    unsigned char status;
    static unsigned char shift = 0;
    char c;

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

        if (scancode == 0x48)
            return (char)ARR_UP;
        if (scancode == 0x50)
            return (char)ARR_DOWN;
        if (scancode == 0x4B)
            return (char)ARR_LEFT;
        if (scancode == 0x4D)
            return (char)ARR_RIGHT;

        if (scancode == 0x1C)
            return '\n';
        if (scancode == 0x0E)
            return '\b';

        switch (scancode)
        {
        case 0x02:
            c = shift ? '!' : '1';
            break;
        case 0x03:
            c = shift ? '@' : '2';
            break;
        case 0x04:
            c = shift ? '#' : '3';
            break;
        case 0x05:
            c = shift ? '$' : '4';
            break;
        case 0x06:
            c = shift ? '%' : '5';
            break;
        case 0x07:
            c = shift ? '^' : '6';
            break;
        case 0x08:
            c = shift ? '&' : '7';
            break;
        case 0x09:
            c = shift ? '*' : '8';
            break;
        case 0x0A:
            c = shift ? '(' : '9';
            break;
        case 0x0B:
            c = shift ? ')' : '0';
            break;
        case 0x0C:
            c = shift ? '_' : '-';
            break;
        case 0x0D:
            c = shift ? '+' : '=';
            break;

        case 0x10:
            c = shift ? 'Q' : 'q';
            break;
        case 0x11:
            c = shift ? 'W' : 'w';
            break;
        case 0x12:
            c = shift ? 'E' : 'e';
            break;
        case 0x13:
            c = shift ? 'R' : 'r';
            break;
        case 0x14:
            c = shift ? 'T' : 't';
            break;
        case 0x15:
            c = shift ? 'Y' : 'y';
            break;
        case 0x16:
            c = shift ? 'U' : 'u';
            break;
        case 0x17:
            c = shift ? 'I' : 'i';
            break;
        case 0x18:
            c = shift ? 'O' : 'o';
            break;
        case 0x19:
            c = shift ? 'P' : 'p';
            break;
        case 0x1A:
            c = shift ? '{' : '[';
            break;
        case 0x1B:
            c = shift ? '}' : ']';
            break;

        case 0x1E:
            c = shift ? 'A' : 'a';
            break;
        case 0x1F:
            c = shift ? 'S' : 's';
            break;
        case 0x20:
            c = shift ? 'D' : 'd';
            break;
        case 0x21:
            c = shift ? 'F' : 'f';
            break;
        case 0x22:
            c = shift ? 'G' : 'g';
            break;
        case 0x23:
            c = shift ? 'H' : 'h';
            break;
        case 0x24:
            c = shift ? 'J' : 'j';
            break;
        case 0x25:
            c = shift ? 'K' : 'k';
            break;
        case 0x26:
            c = shift ? 'L' : 'l';
            break;
        case 0x27:
            c = shift ? ':' : ';';
            break;
        case 0x28:
            c = shift ? '"' : '\'';
            break;

        case 0x2C:
            c = shift ? 'Z' : 'z';
            break;
        case 0x2D:
            c = shift ? 'X' : 'x';
            break;
        case 0x2E:
            c = shift ? 'C' : 'c';
            break;
        case 0x2F:
            c = shift ? 'V' : 'v';
            break;
        case 0x30:
            c = shift ? 'B' : 'b';
            break;
        case 0x31:
            c = shift ? 'N' : 'n';
            break;
        case 0x32:
            c = shift ? 'M' : 'm';
            break;
        case 0x33:
            c = shift ? '<' : ',';
            break;
        case 0x34:
            c = shift ? '>' : '.';
            break;
        case 0x35:
            c = shift ? '?' : '/';
            break;

        case 0x2B:
            c = shift ? '|' : '\\';
            break;
        case 0x39:
            c = ' ';
            break;

        default:
            continue;
        }

        return c;
    }
}