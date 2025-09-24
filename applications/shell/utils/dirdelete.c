#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[31mError: No directory name provided.\033[0m\n");
        return 1;
    }

    const char *dir_name = argv[1];

    if (rmdir(dir_name) == 0)
        printf("\033[32mDirectory deleted successfully.\033[0m\n");
    else
        printf("\033[31mError: Could not delete directory '%s'.\033[0m\n", dir_name);

    return 0;
}
