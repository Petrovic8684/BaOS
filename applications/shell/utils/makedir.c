#include <stdio.h>
#include <sys_stat.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2 || argv[1][0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];

    if (mkdir(name, 0755) == 0)
    {
        printf("\033[32mDirectory created successfully.\033[0m\n");
    }
    else
    {
        printf("\033[31mError: Could not create directory '%s'.\033[0m\n", name);
    }

    return 0;
}
