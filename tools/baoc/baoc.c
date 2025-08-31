#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define LOAD_BASE 0x100000
#define MAX_FILE_SIZE (1024 * 64)
#define MAX_IMAGE (MAX_FILE_SIZE * 4)

unsigned char src_buf[MAX_FILE_SIZE];
unsigned char crt_buf[MAX_FILE_SIZE];
unsigned char image_buf[MAX_IMAGE];
unsigned char code_buf[64];
size_t code_len = 0;

void die(const char *m)
{
    fprintf(stderr, "ERR: %s\n", m);
    exit(1);
}

char *skip_ws(char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

unsigned int my_atoi(const char *s, char **end)
{
    int neg = 0;
    unsigned int val = 0;
    if (*s == '-')
    {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9')
    {
        val = val * 10 + (*s - '0');
        s++;
    }
    if (end)
        *end = (char *)s;
    return neg ? (unsigned int)(-(int)val) : val;
}

void emit_u8(unsigned char b) { code_buf[code_len++] = b; }
void emit_u32(uint32_t v)
{
    memcpy(code_buf + code_len, &v, 4);
    code_len += 4;
}

void compile_minimal(const char *src)
{
    char *p = strstr((char *)src, "int");
    if (!p)
        die("no int in source");
    p = strstr(p, "main");
    if (!p)
        die("no main");
    p = strchr(p, '{');
    if (!p)
        die("no { after main");
    p++;
    p = skip_ws(p);

    int depth = 1;
    int retval = 0;
    int found = 0;
    while (*p && depth > 0)
    {
        if (*p == '{')
        {
            depth++;
            p++;
            continue;
        }
        if (*p == '}')
        {
            depth--;
            p++;
            continue;
        }
        if (strncmp(p, "return", 6) == 0 && isspace((unsigned char)p[6]))
        {
            p += 6;
            p = skip_ws((char *)p);
            if (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)p[1])))
            {
                char *end;
                retval = (int)my_atoi(p, &end);
                p = end;
                found = 1;
            }
            else
                die("unsupported return expression (only integer constants supported)");

            while (*p && *p != ';')
                p++;
            if (*p == ';')
                p++;
            continue;
        }
        p++;
    }

    code_len = 0;
    emit_u8(0xB8);
    emit_u32((uint32_t)retval);
    emit_u8(0xC2);
    emit_u8(0x08);
    emit_u8(0x00);
}

typedef struct
{
    unsigned char e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
    uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} Elf32_Phdr;

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <input.c> <output.bin>\n", argv[0]);
        return 1;
    }

    const char *crt0_path = "/lib/crt0";

    FILE *f = fopen(argv[1], "rb");
    if (!f)
        die("cannot open input");
    size_t src_sz = fread(src_buf, 1, MAX_FILE_SIZE, f);
    fclose(f);
    src_buf[src_sz > 0 ? src_sz : 0] = 0;

    compile_minimal((char *)src_buf);

    FILE *fcrt = fopen(crt0_path, "rb");
    if (!fcrt)
        die("cannot open crt0");
    fseek(fcrt, 0, SEEK_END);
    long crt_sz = ftell(fcrt);
    fseek(fcrt, 0, SEEK_SET);
    if (crt_sz <= 0 || crt_sz > MAX_FILE_SIZE)
        die("crt0 size invalid");
    fread(crt_buf, 1, crt_sz, fcrt);
    fclose(fcrt);

    uint32_t user_code_size = (uint32_t)code_len;
    uint32_t total_user_size = user_code_size;

    uint32_t ph_offset = sizeof(Elf32_Ehdr);
    uint32_t elf_size = ph_offset + sizeof(Elf32_Phdr) + crt_sz + total_user_size;
    if ((size_t)elf_size > MAX_IMAGE)
        die("output too large");
    memset(image_buf, 0, elf_size);

    Elf32_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0] = 0x7F;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 1;
    ehdr.e_ident[5] = 1;
    ehdr.e_ident[6] = 1;
    ehdr.e_type = 2;
    ehdr.e_machine = 3;
    ehdr.e_version = 1;
    ehdr.e_entry = LOAD_BASE;
    ehdr.e_phoff = ph_offset;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_phnum = 1;

    memcpy(image_buf, &ehdr, sizeof(Elf32_Ehdr));

    Elf32_Phdr ph;
    ph.p_type = 1;
    ph.p_offset = ph_offset + sizeof(Elf32_Phdr);
    ph.p_vaddr = LOAD_BASE;
    ph.p_paddr = LOAD_BASE;
    ph.p_filesz = crt_sz + total_user_size;
    ph.p_memsz = crt_sz + total_user_size;
    ph.p_flags = 7;
    ph.p_align = 0x1000;

    memcpy(image_buf + ph_offset, &ph, sizeof(ph));

    memcpy(image_buf + ph_offset + sizeof(Elf32_Phdr), crt_buf, crt_sz);
    memcpy(image_buf + ph_offset + sizeof(Elf32_Phdr) + crt_sz, code_buf, user_code_size);

    int call_offset_in_crt = -1;
    for (int i = 0; i + 4 < crt_sz; ++i)
        if (crt_buf[i] == 0xE8)
        {
            call_offset_in_crt = i;
            break;
        }

    if (call_offset_in_crt >= 0)
    {
        uint32_t call_place_in_file = ph_offset + sizeof(Elf32_Phdr) + (uint32_t)call_offset_in_crt;
        uint32_t call_instr_virt = LOAD_BASE + (uint32_t)call_offset_in_crt;
        uint32_t main_target_virt = LOAD_BASE + (uint32_t)crt_sz;

        int32_t rel32 = (int32_t)(main_target_virt - (call_instr_virt + 5u));
        memcpy(image_buf + call_place_in_file + 1, &rel32, 4);
    }
    else
        fprintf(stderr, "Warning: no CALL opcode found in crt0; crt0 might not contain 'call main'.\n");

    FILE *out = fopen(argv[2], "wb");
    if (!out)
        die("cannot open output");
    fwrite(image_buf, 1, elf_size, out);
    fclose(out);

    printf("Wrote %s (%u bytes) as minimal ELF (no libc).\n", argv[2], elf_size);
    return 0;
}
