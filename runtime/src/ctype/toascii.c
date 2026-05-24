#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int toascii(int c)
{
    if (c == (int)EOF)
        return c;
    return (int)((unsigned char)c & 0x7f);
}
