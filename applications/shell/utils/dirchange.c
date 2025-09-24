#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[31mError: No directory path provided.\033[0m\n");
        return 1;
    }

    const char *path = argv[1];

    if (chdir(path) == 0)
        printf("\033[32mChanged directory successfully.\033[0m\n");
    else
        printf("\033[31mError: could not change directory to '%s'.\033[0m\n", path);

    return 0;
}
