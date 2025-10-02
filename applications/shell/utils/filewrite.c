#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("\033[1;33mUsage:\033[0m filewrite <file> <text>.\n");
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

    FILE *f_check = fopen(name, "rb");
    if (f_check)
    {
        if (fseek(f_check, 0, SEEK_END) != 0)
        {
            printf("\033[31mError: Could not seek file '%s'. %s.\033[0m\n", name, strerror(errno));
            fclose(f_check);
            return 1;
        }

        long fsize = ftell(f_check);
        if (fsize < 0)
        {
            printf("\033[31mError: Could not determine file size for '%s'. %s.\033[0m\n", name, strerror(errno));
            fclose(f_check);
            return 1;
        }
        rewind(f_check);

        unsigned char *buffer = NULL;
        if (fsize > 0)
        {
            buffer = malloc((size_t)fsize);
            if (!buffer)
            {
                printf("\033[31mError: Memory allocation failed. %s.\033[0m\n", strerror(errno));
                fclose(f_check);
                return 1;
            }
        }

        size_t n = fread(buffer, 1, (size_t)fsize, f_check);
        fclose(f_check);

        if (n != (size_t)fsize)
        {
            printf("\033[31mError: Could not read entire file '%s'. %s.\033[0m\n", name, strerror(errno));
            free(buffer);
            return 1;
        }

        int bin_hits = 0;
        for (size_t i = 0; i < n; i++)
        {
            unsigned char c = buffer[i];
            if (c == 0)
                bin_hits += 4;
            else if (c < 32 && c != '\n' && c != '\r' && c != '\t')
                bin_hits++;
        }

        free(buffer);

        if (bin_hits > 0)
        {
            printf("\033[31mError: Existing file '%s' is not a text file.\033[0m\n", name);
            return 1;
        }
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s' for writing. %s.\033[0m\n", name, strerror(errno));
        return 1;
    }

    for (int i = 2; i < argc; ++i)
    {
        if (i > 2)
            fputc(' ', f);
        fputs(argv[i], f);
    }

    fputc('\n', f);

    if (fflush(f) == EOF || fclose(f) != 0)
    {
        printf("\033[31mError: I/O error occurred while writing to file '%s'. %s.\033[0m\n", name, strerror(errno));
        return 1;
    }

    printf("\033[32mFile written successfully.\033[0m\n");
    return 0;
}
