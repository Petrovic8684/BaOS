#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
    if (af != AF_INET || src == NULL || dst == NULL)
        return NULL;

    const unsigned char *bytes = (const unsigned char *)src;
    if (size < INET_ADDRSTRLEN) return NULL;

    int r = snprintf(dst, size, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);

    if (r <= 0 || (size_t)r >= size) return NULL;

    return dst;
}
