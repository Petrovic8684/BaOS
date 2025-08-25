#include "loader.h"

#define USER_STACK_TOP 0x100000

static void jump_to_user(unsigned int entry)
{
    asm volatile(
        "cli\n\t"             // Isključi prekide
        "mov $0x23, %%ax\n\t" // DS, ES, FS, GS = user data segment
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        "pushl $0x23\n\t"    // SS = user data
        "pushl %[stack]\n\t" // ESP = user stack
        "pushf\n\t"          // EFLAGS
        "pushl $0x1B\n\t"    // CS = user code
        "pushl %[entry]\n\t" // EIP = entry point
        "iret\n\t"
        :
        : [entry] "r"(entry), [stack] "r"(USER_STACK_TOP)
        : "ax");
}

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

    write("Jumping to user program in ring3...\n");

    jump_to_user(ehdr->e_entry);
}
