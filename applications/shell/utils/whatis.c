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

    size_t path_len = strlen("/docs/") + strlen(name) + 1;
    char *path = malloc(path_len);
    if (!path)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        return 1;
    }

    snprintf(path, path_len, "/docs/%s", name);

    FILE *f = fopen(path, "rb");
    free(path);
    if (!f)
    {
        printf("\033[31mError: No available docs for program '%s'.\033[0m\n", name);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: Failed to seek file.\033[0m\n");
        fclose(f);
        return 1;
    }

    long fsize = ftell(f);
    if (fsize < 0)
    {
        printf("\033[31mError: Failed to get file size.\033[0m\n");
        fclose(f);
        return 1;
    }
    rewind(f);

    unsigned char *buffer = malloc((size_t)fsize);
    if (!buffer)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        fclose(f);
        return 1;
    }

    size_t n = fread(buffer, 1, (size_t)fsize, f);
    if (n != (size_t)fsize)
    {
        printf("\033[31mError: I/O error occurred while reading docs for '%s'.\033[0m\n", name);
        free(buffer);
        fclose(f);
        return 1;
    }

    printf("\033[2J");
    size_t w = fwrite(buffer, 1, (size_t)fsize, stdout);
    if (w != (size_t)fsize)
    {
        printf("\n\033[31mError: I/O error occurred while writing docs for '%s'.\033[0m\n", name);
        free(buffer);
        fclose(f);
        return 1;
    }

    free(buffer);
    fclose(f);
    return 0;
}
