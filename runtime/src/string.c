#include <string.h>

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

int strcmp(const char *s1, const char *s2)
{
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (*a && *a == *b)
    {
        a++;
        b++;
    }
    return (int)(*a - *b);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--)
    {
        if (*a != *b)
            return (int)(*a - *b);
        if (*a == '\0')
            return 0;
        a++;
        b++;
    }
    return 0;
}

int strcoll(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

size_t strxfrm(char *dest, const char *src, size_t n)
{
    size_t len = strlen(src);

    if (n > 0)
    {
        size_t copy = (len < n - 1) ? len : (n - 1);
        for (size_t i = 0; i < copy; i++)
            dest[i] = src[i];

        dest[copy] = '\0';
    }

    return len;
}

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++) != '\0')
        return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d)
        d++;
    while ((*d++ = *src++) != '\0')
        ;
    return dest;
}

char *strncat(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (*d)
        d++;
    while (n-- && *src)
    {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

char *strchr(const char *s, int c)
{
    unsigned char uc = (unsigned char)c;
    for (;; s++)
    {
        if ((unsigned char)*s == uc)
            return (char *)s;
        if (*s == '\0')
            break;
    }
    return NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    unsigned char uc = (unsigned char)c;
    for (; *s; s++)
    {
        if ((unsigned char)*s == uc)
            last = s;
    }
    if (uc == '\0')
        return (char *)s;
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;
    for (; *haystack; haystack++)
    {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n)
        {
            h++;
            n++;
        }
        if (*n == '\0')
            return (char *)haystack;
    }
    return NULL;
}

size_t strspn(const char *s, const char *accept)
{
    size_t count = 0;
    const char *p;
    for (; *s; s++)
    {
        for (p = accept; *p; p++)
        {
            if (*s == *p)
                break;
        }
        if (*p == '\0')
            break;
        count++;
    }
    return count;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t count = 0;
    const char *p;
    for (; *s; s++)
    {
        for (p = reject; *p; p++)
        {
            if (*s == *p)
                return count;
        }
        count++;
    }
    return count;
}

char *strpbrk(const char *s, const char *accept)
{
    const char *p;
    for (; *s; s++)
    {
        for (p = accept; *p; p++)
        {
            if (*s == *p)
                return (char *)s;
        }
    }
    return NULL;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s)
    {
        while (n--)
            *d++ = *s++;
    }
    else if (d > s)
    {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char uc = (unsigned char)c;
    while (n--)
    {
        if (*p == uc)
            return (void *)p;
        p++;
    }
    return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--)
    {
        if (*p1 != *p2)
            return (int)(*p1 - *p2);
        p1++;
        p2++;
    }
    return 0;
}

char *strtok_r(char *s, const char *delim, char **saveptr)
{
    char *start;
    char *p;

    if (s == NULL)
    {
        s = *saveptr;
        if (s == NULL)
            return NULL;
    }

    for (; *s; s++)
    {
        const char *d = delim;
        int is_delim = 0;
        for (; *d; d++)
            if (*s == *d)
            {
                is_delim = 1;
                break;
            }
        if (!is_delim)
            break;
    }

    if (*s == '\0')
    {
        *saveptr = NULL;
        return NULL;
    }

    start = s;

    for (p = s; *p; p++)
    {
        const char *d = delim;
        for (; *d; d++)
            if (*p == *d)
                break;
        if (*d)
        {
            *p = '\0';
            *saveptr = p + 1;
            return start;
        }
    }

    *saveptr = NULL;
    return start;
}

char *strtok(char *s, const char *delim)
{
    static char *saveptr;
    return strtok_r(s, delim, &saveptr);
}