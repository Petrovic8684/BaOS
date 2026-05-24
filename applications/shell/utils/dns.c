#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("\033[1;33mUsage:\033[0m dns <hostname>.\n");
        return 1;
    }

    const char *hostname = argv[1];
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL)
    {
        printf("\033[31mError: Lookup failed for %s.\033[0m\n", hostname);
        return 1;
    }

    printf("\033[1;33mIPv4 addresses for %s:\033[0m\n", hostname);

    for (int i = 0; he->h_addr_list[i] != NULL; i++)
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, he->h_addr_list[i], ip, sizeof(ip));
        
        printf("  %s\n", ip);
    }

    for (int i = 0; he->h_addr_list[i] != NULL; i++)
        free(he->h_addr_list[i]);

    free(he->h_addr_list);
    free(he->h_name);
    free(he); 

    return 0;
}
