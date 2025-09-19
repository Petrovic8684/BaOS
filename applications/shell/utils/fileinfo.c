#include "./common/fs_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
        printf("\033[31mError: Could not open file '%s'.\n\033[0m", fname);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: File seek failed.\n\033[0m");
        fclose(f);
        return 1;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        printf("\033[31mError: File tell failed.\n\033[0m");
        fclose(f);
        return 1;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: File seek failed.\n\033[0m");
        fclose(f);
        return 1;
    }

    unsigned int size = (unsigned int)sz;

    char location[512];
    normalize_path(fname, location, 512);
    fname = path_basename(fname);

    printf("Filename: %s\n", fname);
    printf("Location: %s\n", location);
    printf("Size: %u bytes\n", size);

    unsigned int to_read = (size < 512u) ? size : 512u;
    unsigned char buf[512];
    unsigned int out_read = 0;

    if (to_read > 0)
    {
        size_t n = fread(buf, 1, (size_t)to_read, f);
        out_read = (unsigned int)n;
    }

    fclose(f);

    if (out_read >= 4 && buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F')
        printf("Type: ELF executable.\n");

    else if (out_read >= 2 && buf[0] == '#' && buf[1] == '!')
        printf("Type: BaOSh script.\n");

    else if (out_read >= 8 && memcmp(buf, "\x89PNG\r\n\x1A\n", 8) == 0)
        printf("Type: PNG image.\n");

    else if (out_read >= 3 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
        printf("Type: JPEG image.\n");

    else if (out_read >= 6 && (memcmp(buf, "GIF87a", 6) == 0 || memcmp(buf, "GIF89a", 6) == 0))
        printf("Type: GIF image.\n");

    else if (out_read >= 4 && memcmp(buf, "%PDF", 4) == 0)
        printf("Type: PDF document.\n");

    else if (out_read >= 4 && memcmp(buf, "PK\x03\x04", 4) == 0)
        printf("Type: ZIP archive (or OOXML/JAR/...).\n");

    else if (is_likely_text(buf, out_read))
        printf("Type: Text file.\n");

    else
        printf("Type: Unknown / binary.\n");

    return 0;
}
