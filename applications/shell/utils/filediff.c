#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("\033[31mError: Invalid source or target provided.\033[0m\n");
        return 1;
    }

    const char *file1 = argv[1];
    const char *file2 = argv[2];

    FILE *f1 = fopen(file1, "rb");
    if (!f1)
    {
        printf("\033[31mError: Could not open file '%s'.\033[0m\n", file1);
        return 1;
    }

    FILE *f2 = fopen(file2, "rb");
    if (!f2)
    {
        printf("\033[31mError: Could not open file '%s'.\033[0m\n", file2);
        fclose(f1);
        return 1;
    }

    if (fseek(f1, 0, SEEK_END) != 0 || fseek(f2, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: Could not seek files.\033[0m\n");
        fclose(f1);
        fclose(f2);
        return 1;
    }

    long sz1 = ftell(f1);
    long sz2 = ftell(f2);
    if (sz1 < 0 || sz2 < 0)
    {
        printf("\033[31mError: Could not tell file sizes.\033[0m\n");
        fclose(f1);
        fclose(f2);
        return 1;
    }

    if (sz1 == 0 && sz2 == 0)
    {
        printf("\033[32mFiles are identical (both empty).\033[0m\n");
        fclose(f1);
        fclose(f2);
        return 1;
    }

    if ((sz1 == 0 && sz2 != 0) || (sz1 != 0 && sz2 == 0) || sz1 != sz2)
    {
        printf("\033[1;33mFiles differ (size mismatch).\033[0m\n");
        fclose(f1);
        fclose(f2);
        return 1;
    }

    if (fseek(f1, 0, SEEK_SET) != 0 || fseek(f2, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: Could not rewind files.\033[0m\n");
        fclose(f1);
        fclose(f2);
        return 1;
    }

    unsigned char *buf1 = (unsigned char *)malloc((size_t)sz1);
    unsigned char *buf2 = (unsigned char *)malloc((size_t)sz2);
    if (!buf1 || !buf2)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        free(buf1);
        free(buf2);
        fclose(f1);
        fclose(f2);
        return 1;
    }

    size_t n1 = fread(buf1, 1, (size_t)sz1, f1);
    size_t n2 = fread(buf2, 1, (size_t)sz2, f2);
    fclose(f1);
    fclose(f2);

    if (n1 != (size_t)sz1 || n2 != (size_t)sz2)
    {
        printf("\033[31mError: Could not read entire files.\033[0m\n");
        free(buf1);
        free(buf2);
        return 1;
    }

    int diff = 0;
    for (size_t i = 0; i < n1; i++)
        if (buf1[i] != buf2[i])
        {
            diff = 1;
            char c1 = (buf1[i] >= 32 && buf1[i] <= 126) ? buf1[i] : '.';
            char c2 = (buf2[i] >= 32 && buf2[i] <= 126) ? buf2[i] : '.';
            printf("\033[1;33mFiles differ at byte %u (0x%x '%c' vs 0x%x '%c')\033[0m\n", i, buf1[i], c1, buf2[i], c2);

            break;
        }

    if (!diff)
        printf("\033[32mFiles are identical.\033[0m\n");

    free(buf1);
    free(buf2);
    return diff;
}
