#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdbool.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 2 || argv[1][0] == '\0')
    {
        printf("\033[1;33mUsage:\033[0m filemake <file>.\n");
        return 1;
    }

    const char *name = argv[1];
    char *name_base = basename(name);

    if (strlen(name_base) > 16)
    {
        printf("\033[31mError: Name exceeds maximum length of 16 characters.\033[0m\n");
        return 1;
    }

    bool all_dots = true;
    for (size_t i = 0; i < strlen(name_base); i++)
    {
        if (!isalnum((unsigned char)name_base[i]) && (unsigned char)name_base[i] != '.')
        {
            printf("\033[31mError: Name must contain only letters, digits and dots, and cannot consist of dots only.\033[0m\n");
            return 1;
        }
        if (name_base[i] != '.')
            all_dots = false;
    }

    if (all_dots)
    {
        printf("\033[31mError: Name must contain only letters, digits and dots, and cannot consist of dots only.\033[0m\n");
        return 1;
    }

    char *dup = strdup(name);
    if (!dup)
    {
        printf("\033[31mError: Out of memory. %s.\033[0m\n", strerror(errno));
        return 1;
    }

    char *parent = dirname(dup);
    if (parent && parent[0] != '\0')
    {
        DIR *d = opendir(parent);
        if (!d)
        {
            printf("\033[31mError: Parent directory '%s' does not exist. %s.\033[0m\n", parent, strerror(errno));
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
        printf("\033[31mError: Could not create file '%s'. %s.\033[0m\n", name, strerror(errno));
        return 1;
    }

    if (fclose(f) != 0)
    {
        printf("\033[31mError: Failed to finalize file '%s'. %s.\033[0m\n", name, strerror(errno));
        return 1;
    }

    printf("\033[32mFile created successfully.\033[0m\n");
    return 0;
}
