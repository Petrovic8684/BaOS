#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int main()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd != NULL)
    {
        printf("%s\n", cwd);
        return 0;
    }

    printf("\033[31mError: Could not retrieve current directory. %s.\033[0m\n", strerror(errno));
    return 1;
}
