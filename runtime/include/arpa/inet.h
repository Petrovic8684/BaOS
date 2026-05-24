#ifndef ARPA_INET_H
#define ARPA_INET_H

#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#include <stdint.h>

typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
};

const char *inet_ntop(int af, const void *src, char *dst, size_t size);

#endif
