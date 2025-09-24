#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[31mError: Invalid source provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];
    FILE *f = fopen(name, "rb");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s'.\033[0m\n", name);
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
        printf("\033[31mError: Could not determine file size.\033[0m\n");
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

    long lines = 0;
    long words = 0;
    long chars = 0;
    int in_word = 0;
    int bin_hits = 0;

    for (size_t i = 0; i < n; i++)
    {
        unsigned char c = buffer[i];
        chars++;

        if (c == '\n')
            lines++;

        if (isspace(c))
            in_word = 0;
        else if (!in_word)
        {
            in_word = 1;
            words++;
        }

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

    printf("\033[1;33mFile count:\033[0m\n\n");

    printf("\033[1;33mCharacters:\033[0m %d\n", chars);
    printf("\033[1;33mWords:\033[0m %d\n", words);
    printf("\033[1;33mLines:\033[0m %d\n", lines);

    free(buffer);
    return 0;
}
