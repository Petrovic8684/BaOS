#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int tolower(int c)
{
    if (c == (int)EOF)
        return c;
    unsigned char uc = (unsigned char)c;
    if (uc >= 'A' && uc <= 'Z')
        return (int)(uc - 'A' + 'a');
    return (int)uc;
}
