#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <libgen.h>

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
        if (!isalnum((unsigned char)name[i]) && (unsigned char)name[i] != '.')
        {
            printf("\033[31mError: Name must contain only letters and digits.\033[0m\n");
            return 1;
        }

    char *dup = strdup(name);
    if (!dup)
    {
        printf("\033[31mError: Out of memory.\033[0m\n");
        return 1;
    }

    char *parent = dirname(dup);
    if (parent && parent[0] != '\0')
    {
        DIR *d = opendir(parent);
        if (!d)
        {
            printf("\033[31mError: Parent directory '%s' does not exist.\033[0m\n", parent);
            free(dup);
            return 1;
        }
        closedir(d);
    }
    free(dup);

    FILE *check = fopen(name, "r");
    if (check)
    {
        fclose(check);
        printf("\033[31mError: File '%s' already exists.\033[0m\n", name);
        return 1;
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("\033[31mError: Could not create file '%s'.\033[0m\n", name);
        return 1;
    }

    if (fclose(f) != 0)
    {
        printf("\033[31mError: Failed to finalize file '%s'.\033[0m\n", name);
        return 1;
    }

    printf("\033[32mFile created successfully.\033[0m\n");
    return 0;
}
