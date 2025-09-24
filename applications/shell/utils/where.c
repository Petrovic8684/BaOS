#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd != NULL)
    {
        printf("%s\n", cwd);
        free(cwd);
    }
    else
    {
        printf("\033[31mError: Could not retrieve current directory.\033[0m\n");
        return 1;
    }

    return 0;
}
