#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("\033[31mError: Invalid name or text provided.\033[0m\n");
        return 1;
    }

    const char *name = argv[1];
    const char *text = argv[2];

    FILE *f_check = fopen(name, "rb");
    if (f_check)
    {
        unsigned char buffer[256];
        size_t n;
        int bin_hits = 0;

        n = fread(buffer, 1, sizeof(buffer), f_check);
        for (size_t i = 0; i < n; i++)
        {
            unsigned char c = buffer[i];
            if (c == 0)
                bin_hits += 4;
            else if (c < 32 && c != '\n' && c != '\r' && c != '\t')
                bin_hits++;
        }

        fclose(f_check);

        if (bin_hits > 0)
        {
            printf("\033[31mError: Existing file '%s' is not a text file.\033[0m\n", name);
            return 1;
        }
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s' for writing.\033[0m\n", name);
        return 1;
    }

    size_t len = 0;
    while (text[len])
        len++;

    size_t written = fwrite(text, 1, len, f);
    if (written != len)
    {
        printf("\033[31mError: I/O error occurred while writing to file '%s'.\033[0m\n", name);
        fclose(f);
        return 1;
    }

    const char newline = '\n';
    if (fwrite(&newline, 1, 1, f) != 1)
    {
        printf("\033[31mError: Failed to write newline to file '%s'.\033[0m\n", name);
        fclose(f);
        return 1;
    }

    if (fflush(f) == EOF)
    {
        printf("\033[31mError: I/O error occurred while writing to file '%s'.\033[0m\n", name);
        fclose(f);
        return 1;
    }

    if (fclose(f) != 0)
    {
        printf("\033[31mError: Failed to close file '%s' after writing.\033[0m\n", name);
        return 1;
    }

    printf("\033[32mFile written successfully.\033[0m\n");
    return 0;
}
