#include <ctype.h>
#include <stdio.h>

int isalnum(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z'));
}

int isalpha(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z'));
}

int iscntrl(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc < 0x20 || uc == 0x7f);
}

int isdigit(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc >= '0' && uc <= '9');
}

int isgraph(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc > 0x20 && uc < 0x7f);
}

int islower(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc >= 'a' && uc <= 'z');
}

int isupper(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc >= 'A' && uc <= 'Z');
}

int ispunct(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    if (uc <= 0x20 || uc >= 0x7f)
        return 0;
    if ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z') || uc == ' ')
        return 0;
    return 1;
}

int isspace(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc == ' ' || uc == '\f' || uc == '\n' || uc == '\r' || uc == '\t' || uc == '\v');
}

int isprint(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc >= 0x20 && uc < 0x7f);
}

int isxdigit(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'F') || (uc >= 'a' && uc <= 'f'));
}

int tolower(int c)
{
    if (c == (int)EOF)
        return c;
    unsigned char uc = (unsigned char)c;
    if (uc >= 'A' && uc <= 'Z')
        return (int)(uc - 'A' + 'a');
    return (int)uc;
}

int toupper(int c)
{
    if (c == (int)EOF)
        return c;
    unsigned char uc = (unsigned char)c;
    if (uc >= 'a' && uc <= 'z')
        return (int)(uc - 'a' + 'A');
    return (int)uc;
}