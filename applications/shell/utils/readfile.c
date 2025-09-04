#include <stdio.h>

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

    unsigned char buffer[256];
    size_t n;
    int any = 0;
    int bin_hits = 0;

    n = fread(buffer, 1, sizeof(buffer), f);
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
        fclose(f);
        return 1;
    }

    rewind(f);

    size_t printed = 0;
    int truncated = 0;

    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (printed >= 120)
            {
                truncated = 1;
                break;
            }
            putchar(buffer[i]);
            printed++;
            any = 1;
        }
        if (truncated)
            break;
    }

    if (ferror(f))
    {
        printf("\n\033[31mError: I/O error occurred while reading file '%s'.\033[0m\n", name);
        fclose(f);
        return 1;
    }

    if (!any)
        printf("(empty file)\n");
    else if (truncated)
        printf("... (truncated)\n");

    fclose(f);
    return 0;
}
