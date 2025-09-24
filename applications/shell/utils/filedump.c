#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("\033[31mError: Invalid source provided.\033[0m\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("\033[31mError: Cannot open file: %s.\n\033[0m\n", argv[1]);
        return 1;
    }

    unsigned char buf[16];
    int offset = 0;
    size_t n;

    while ((n = fread(buf, 1, 16, f)) > 0)
    {
        printf("\033[1;33m%x  \033[0m", offset);

        for (size_t i = 0; i < 16; i++)
        {
            if (i < n)
            {
                unsigned char byte = buf[i];
                printf("%x", byte / 16);
                printf("%x ", byte % 16);
            }
            else
                printf("   ");
        }

        printf("|");

        for (size_t i = 0; i < n; i++)
        {
            unsigned char c = buf[i];
            if (c >= 32 && c <= 126)
                printf("%c", c);
            else
                printf(".");
        }

        printf("|\n");
        offset += 16;
    }

    fclose(f);
    return 0;
}
