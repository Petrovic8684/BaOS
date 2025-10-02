#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[1;33mUsage:\033[0m dirdelete <directory>.\n");
        return 1;
    }

    const char *dir_name = argv[1];
    char *resolved_target = realpath(dir_name, NULL);
    if (!resolved_target)
    {
        printf("\033[31mError: Cannot resolve path '%s'. %s.\033[0m\n", dir_name, strerror(errno));
        return 1;
    }

    char *resolved_cwd = getcwd(NULL, 0);
    if (!resolved_cwd)
    {
        printf("\033[31mError: Cannot get current working directory. %s.\033[0m\n", strerror(errno));
        free(resolved_target);
        return 1;
    }

    if (strcmp(resolved_target, resolved_cwd) == 0)
    {
        printf("\033[31mError: Refusing to delete current working directory.\033[0m\n");
        free(resolved_target);
        return 1;
    }

    if (rmdir(resolved_target) == 0)
        printf("\033[32mDirectory deleted successfully.\033[0m\n");
    else
        printf("\033[31mError: Could not delete directory '%s'. %s.\033[0m\n", resolved_target, strerror(errno));

    free(resolved_target);
    return 0;
}
