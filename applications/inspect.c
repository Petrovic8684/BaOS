#include <stdio.h>
#include <stdint.h>

#pragma pack(push, 1)
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
#pragma pack(pop)

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <elf.bin>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("fopen");
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        printf("fseek");
        fclose(f);
        return 1;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        printf("ftell");
        fclose(f);
        return 1;
    }
    printf("FILE SIZE = %ld bytes\n", sz);
    rewind(f);

    Elf32_Ehdr eh;
    if (fread(&eh, 1, sizeof(eh), f) != sizeof(eh))
    {
        fprintf(stderr, "Failed to read ELF header (file too small?)\n");
        fclose(f);
        return 1;
    }

    printf("ELF Header:\n");
    printf("  e_phoff = %u\n", eh.e_phoff);
    printf("  e_phentsize = %u\n", eh.e_phentsize);
    printf("  e_phnum = %u\n", eh.e_phnum);
    printf("  e_entry = 0x%X\n", eh.e_entry);

    if (fseek(f, eh.e_phoff, SEEK_SET) != 0)
    {
        printf("fseek ph");
        fclose(f);
        return 1;
    }

    for (int i = 0; i < eh.e_phnum; ++i)
    {
        Elf32_Phdr ph;
        if (fread(&ph, 1, sizeof(ph), f) != sizeof(ph))
        {
            fprintf(stderr, "Failed to read program header %d\n", i);
            fclose(f);
            return 1;
        }
        printf("PHDR %d:\n", i);
        printf("  p_type   = 0x%X\n", ph.p_type);
        printf("  p_offset = 0x%X (%u)\n", ph.p_offset, ph.p_offset);
        printf("  p_vaddr  = 0x%X\n", ph.p_vaddr);
        printf("  p_filesz = %u\n", ph.p_filesz);
        printf("  p_memsz  = %u\n", ph.p_memsz);
        printf("  p_flags  = 0x%X\n", ph.p_flags);
    }

    fclose(f);
    return 0;
}
