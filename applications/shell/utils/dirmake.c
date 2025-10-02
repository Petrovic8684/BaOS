#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 2 || argv[1][0] == '\0')
    {
        printf("\033[1;33mUsage:\033[0m dirmake <directory>.\n");
        return 1;
    }

    const char *name = argv[1];
    char *name_base = basename(name);

    if (strlen(name_base) > 16)
    {
        printf("\033[31mError: Name exceeds maximum length of 16 characters.\033[0m\n");
        return 1;
    }
    for (size_t i = 0; i < strlen(name_base); i++)
        if (!isalnum((unsigned char)name_base[i]))
        {
            printf("\033[31mError: Name must contain only letters and digits.\033[0m\n");
            return 1;
        }

    if (mkdir(name, 0755) == 0)
        printf("\033[32mDirectory created successfully.\033[0m\n");

    else
        printf("\033[31mError: Could not create directory '%s'. %s.\033[0m\n", name, strerror(errno));

    return 0;
}
