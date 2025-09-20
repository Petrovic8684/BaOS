#include <stdio.h>
#include <sys_stat.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    if (argc < 2 || argv[1][0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];

    if (strlen(name) > 16)
    {
        printf("\033[31mError: Name exceeds maximum length of 16 characters.\033[0m\n");
        return 1;
    }

    for (size_t i = 0; i < strlen(name); i++)
        if (!isalnum((unsigned char)name[i]))
        {
            printf("\033[31mError: Name must contain only letters and digits.\033[0m\n");
            return 1;
        }

    if (mkdir(name, 0755) == 0)
        printf("\033[32mDirectory created successfully.\033[0m\n");

    else
        printf("\033[31mError: Could not create directory '%s'.\033[0m\n", name);

    return 0;
}
