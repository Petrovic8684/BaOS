#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];
    FILE *f = fopen(name, "rb");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s' for reading.\033[0m\n", name);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: Could not seek file.\033[0m\n");
        fclose(f);
        return 1;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        printf("\033[31mError: Could not tell file size.\033[0m\n");
        fclose(f);
        return 1;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: Could not rewind file.\033[0m\n");
        fclose(f);
        return 1;
    }

    if (sz == 0)
    {
        printf("\033[1;33m(empty file)\033[0m\n");
        fclose(f);
        return 0;
    }

    unsigned char *buffer = (unsigned char *)malloc((size_t)sz);
    if (!buffer)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        fclose(f);
        return 1;
    }

    size_t n = fread(buffer, 1, (size_t)sz, f);
    if (n != (size_t)sz)
    {
        printf("\033[31mError: Could not read entire file.\033[0m\n");
        free(buffer);
        fclose(f);
        return 1;
    }

    fclose(f);

    int bin_hits = 0;
    for (size_t i = 0; i < n; i++)
    {
        unsigned char c = buffer[i];
        if (c == 0)
            bin_hits += 4;
        else if (c < 32 && c != '\n' && c != '\r' && c != '\t')
            bin_hits++;
    }

    if (bin_hits > 0)
    {
        printf("\033[31mError: Not a text file.\033[0m\n");
        free(buffer);
        return 1;
    }

    fwrite(buffer, 1, n, stdout);
    if (n > 0 && buffer[n - 1] != '\n')
        putchar('\n');

    free(buffer);
    return 0;
}
