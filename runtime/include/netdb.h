#ifndef NETDB_H
#define NETDB_H

#include <stddef.h>

extern int h_errno;

#define HOST_NOT_FOUND  1
#define TRY_AGAIN       2
#define NO_RECOVERY     3
#define NO_DATA         4

struct hostent {
    char  *h_name;
    char **h_aliases;
    int    h_addrtype;
    int    h_length;
    char **h_addr_list;
};

#define h_addr h_addr_list[0]

struct hostent *gethostbyname(const char *name);

#endif
