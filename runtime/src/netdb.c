#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

int h_errno = 0;

struct hostent *gethostbyname(const char *name)
{
    if (!name)
    {
        h_errno = HOST_NOT_FOUND;
        return NULL;
    }

    struct hostent *he = malloc(sizeof(*he));
    if (!he)
    {
        h_errno = NO_RECOVERY;
        return NULL;
    }

    he->h_name = strdup(name);
    he->h_aliases = NULL;
    he->h_addrtype = AF_INET;
    he->h_length = 4;

    char **addr_list = malloc(2 * sizeof(char *));
    if (!addr_list)
    {
        free(he->h_name);
        free(he);
        h_errno = NO_RECOVERY;

        return NULL;
    }

    unsigned char *addr = malloc(4);
    if (!addr)
    {
        free(addr_list);
        free(he->h_name);
        free(he);
        h_errno = NO_RECOVERY;

        return NULL;
    }

    addr[0] = 93;
    addr[1] = 184;
    addr[2] = 216;
    addr[3] = 34;

    addr_list[0] = (char *)addr;
    addr_list[1] = NULL;

    he->h_addr_list = addr_list;

    h_errno = 0;
    return he;
}
