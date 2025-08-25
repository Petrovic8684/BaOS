#include "loader.h"

void load_user_program(const char *name)
{
    if (!fs_initialized)
    {
        write("FS not initialized.\n");
        return;
    }

    unsigned char buf[8192];
    unsigned int size = 0;

    if (fs_read_file_buffer(name, buf, sizeof(buf), &size) != FS_OK)
    {
        write("Failed to read user program.\n");
        return;
    }

    if (size == 0)
    {
        write("User program empty.\n");
        return;
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;

    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
    {
        write("Not a valid ELF file.\n");
        return;
    }

    Elf32_Phdr *phdr = (Elf32_Phdr *)(buf + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        unsigned char *dest = (unsigned char *)(phdr[i].p_paddr);
        unsigned char *src = buf + phdr[i].p_offset;

        for (Elf32_Word j = 0; j < phdr[i].p_filesz; j++)
            dest[j] = src[j];

        for (Elf32_Word j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++)
            dest[j] = 0;
    }

    write("Jumping to user program...\n");

    asm volatile("movl $0x30000, %esp");

    void (*entry)() = (void (*)())(ehdr->e_entry);
    entry();
}
