#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2 || argv[1][0] == '\0')
    {
        printf("\033[31mError: No program name provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];
    char path[512];
    int needed = snprintf(path, sizeof(path), "/docs/%s", name);
    if (needed < 0 || (size_t)needed >= sizeof(path))
    {
        printf("\033[31mError: Docs path too long.\033[0m\n");
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("\033[31mError: No available docs for program '%s'.\033[0m\n", name);
        return 1;
    }

    unsigned char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        printf("\033[2J");
        size_t w = fwrite(buffer, 1, n, stdout);
        if (w != n)
        {
            printf("\n\033[31mError: I/O error occurred while writing docs for '%s'.\033[0m\n", name);
            fclose(f);
            return 1;
        }
    }

    if (ferror(f))
    {
        printf("\n\033[31mError: I/O error occurred while reading docs for '%s'.\033[0m\n", name);
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}
