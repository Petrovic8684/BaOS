#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("\033[1;33mUsage:\033[0m filesearch <file> <text>.\n");
        return 1;
    }

    const char *filename = argv[1];
    const char *pattern = argv[2];

    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s'. %s.\033[0m\n", filename, strerror(errno));
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: Could not seek file. %s.\033[0m\n", strerror(errno));
        fclose(f);
        return 1;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        printf("\033[31mError: Could not tell file size. %s.\033[0m\n", strerror(errno));
        fclose(f);
        return 1;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: Could not rewind file. %s.\033[0m\n", strerror(errno));
        fclose(f);
        return 1;
    }

    if (sz == 0)
    {
        printf("\033[1;33m(empty file)\033[0m\n");
        fclose(f);
        return 0;
    }

    unsigned char *buffer = (unsigned char *)malloc((size_t)sz + 1);
    if (!buffer)
    {
        printf("\033[31mError: Memory allocation failed. %s.\033[0m\n", strerror(errno));
        fclose(f);
        return 1;
    }

    size_t n = fread(buffer, 1, (size_t)sz, f);
    fclose(f);
    buffer[n] = '\0';

    if (n != (size_t)sz)
    {
        printf("\033[31mError: Could not read entire file. %s.\033[0m\n", strerror(errno));
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

    if (bin_hits > 0)
    {
        printf("\033[31mError: Not a text file.\033[0m\n");
        free(buffer);
        return 1;
    }

    char *saveptr;
    char *line = strtok_r((char *)buffer, "\n", &saveptr);
    while (line)
    {
        char *pos = line;
        char *match;
        int found = 0;

        while ((match = strstr(pos, pattern)) != NULL)
        {
            found = 1;
            fwrite(pos, 1, (size_t)(match - pos), stdout);
            printf("\033[32m%s\033[0m", pattern);
            pos = match + strlen(pattern);
        }

        if (found)
            printf("%s\n", pos);

        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(buffer);
    return 0;
}
