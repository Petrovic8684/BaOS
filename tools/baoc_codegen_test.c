#include "../applications/baoc/internal/baoc_internal.h"
#include <stdio.h>
#include <string.h>

int file_read_all(const char *path, void *dest, size_t dest_max, size_t *out_sz)
{
    (void)path;
    (void)dest;
    (void)dest_max;
    if (out_sz)
        *out_sz = 0;
    return -1;
}

int file_write_all(const char *path, const void *data, size_t sz)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    if (fwrite(data, 1, sz, f) != sz)
    {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static const char test3_src[] =
    "int add(int a, int b)\n"
    "{\n"
    "    return a + b;\n"
    "}\n"
    "\n"
    "int main()\n"
    "{\n"
    "    int arr[3];\n"
    "    int *p;\n"
    "    int i;\n"
    "\n"
    "    arr[0] = 1;\n"
    "    arr[1] = 4;\n"
    "    arr[2] = 9;\n"
    "\n"
    "    p = arr;\n"
    "    i = p[1];\n"
    "\n"
    "    return add(i, 5);\n"
    "}\n";

int main(void)
{
    if (baoc_memory_init() != 0)
        return 1;

    baoc_compile_reset();
    compile(test3_src);

    static unsigned char crt0[] = {
        0x8B, 0x04, 0x24, 0x8B, 0x5C, 0x24, 0x04, 0x53, 0x50,
        0xE8, 0x00, 0x00, 0x00, 0x00, 0x89, 0xC3, 0xB8, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x80,
    };
    memcpy(crt_buf, crt0, sizeof(crt0));
    crt_sz = sizeof(crt0);
    libc_sz = 0;
    syms_sz = 0;
    baoc_link_elf("tools/out3_host.bin");

    printf("code_len=%zu elf=%zu\n", code_len, (size_t)(52 + 32 + sizeof(crt0) + code_len));
    for (size_t i = 0; i < code_len; ++i)
        printf("%02x%c", code_buf[i], ((i + 1) % 16) ? ' ' : '\n');
    if (code_len % 16)
        putchar('\n');

    baoc_memory_free();
    return 0;
}
