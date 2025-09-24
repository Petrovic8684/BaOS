#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

static int is_likely_text(const unsigned char *buf, unsigned int len)
{
    if (len == 0)
        return 1;

    unsigned int printable = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        unsigned char c = buf[i];
        if (c == 9 || c == 10 || c == 13 || (c >= 0x20 && c <= 0x7E))
            printable++;
    }

    return (printable * 100u) > (90u * len);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[31mError: No file name provided.\033[0m\n");
        return 1;
    }

    const char *fname = argv[1];
    FILE *f = fopen(fname, "rb");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s'.\033[0m\n", fname);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: File seek failed.\033[0m\n");
        fclose(f);
        return 1;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        printf("\033[31mError: File tell failed.\033[0m\n");
        fclose(f);
        return 1;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: File seek failed.\033[0m\n");
        fclose(f);
        return 1;
    }

    unsigned int size = (unsigned int)sz;

    char *location = realpath(fname, NULL);
    if (!location)
    {
        printf("\033[31mError: Could not resolve absolute path for '%s'.\033[0m\n", fname);
        fclose(f);
        return 1;
    }

    char *loc_copy = strdup(location);
    if (!loc_copy)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        fclose(f);
        free(location);
        return 1;
    }

    char *base = basename(loc_copy);

    printf("\033[1;33m--- FILE INFO ---\033[0m\n\n");

    printf("\033[1;33mFilename:\033[0m %s\n", base);
    printf("\033[1;33mLocation:\033[0m %s\n", location);
    printf("\033[1;33mSize:\033[0m     %u bytes\n", size);

    if (size == 0)
    {
        printf("\033[1;33mType:\033[0m     Empty file.\n");
        fclose(f);
        free(loc_copy);
        free(location);
        return 0;
    }

    unsigned int to_read = (size < 512u) ? size : 512u;
    unsigned char *buf = (unsigned char *)malloc(to_read);
    if (!buf)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        fclose(f);
        free(loc_copy);
        free(location);
        return 1;
    }

    unsigned int out_read = 0;
    if (to_read > 0)
    {
        size_t n = fread(buf, 1, (size_t)to_read, f);
        out_read = (unsigned int)n;
    }

    fclose(f);

    if (out_read >= 4 && buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F')
        printf("\033[1;33mType:\033[0m     ELF executable.\n");

    else if (out_read >= 2 && buf[0] == '#' && buf[1] == '!')
        printf("\033[1;33mType:\033[0m     BaOSh script.\n");

    else if (out_read >= 8 && memcmp(buf, "\x89PNG\r\n\x1A\n", 8) == 0)
        printf("\033[1;33mType:\033[0m     PNG image.\n");

    else if (out_read >= 3 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
        printf("\033[1;33mType:\033[0m     JPEG image.\n");

    else if (out_read >= 6 && (memcmp(buf, "GIF87a", 6) == 0 || memcmp(buf, "GIF89a", 6) == 0))
        printf("\033[1;33mType:\033[0m     GIF image.\n");

    else if (out_read >= 4 && memcmp(buf, "%PDF", 4) == 0)
        printf("\033[1;33mType:\033[0m     PDF document.\n");

    else if (out_read >= 4 && memcmp(buf, "PK\x03\x04", 4) == 0)
        printf("\033[1;33mType:\033[0m     ZIP archive (or OOXML/JAR/...).\n");

    else if (is_likely_text(buf, out_read))
        printf("\033[1;33mType:\033[0m     Text file.\n");

    else
        printf("\033[1;33mType:\033[0m     Unknown binary.\n");

    free(buf);
    free(loc_copy);
    free(location);

    return 0;
}
