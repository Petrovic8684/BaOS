#include "loader.h"
#include "../paging/paging.h"
#include "../drivers/display/display.h"
#include "../fs/fs.h"

#define PAGE_SIZE 4096
#define USER_STACK_TOP 0x200000
#define USER_STACK_PAGES 4

static void jump_to_user(unsigned int entry)
{
    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        "pushl $0x23\n\t"
        "pushl %[stack]\n\t"
        "pushf\n\t"
        "pushl $0x1B\n\t"
        "pushl %[entry]\n\t"
        "iret\n\t"
        :
        : [entry] "r"(entry), [stack] "r"(USER_STACK_TOP)
        : "ax");
}

uint32_t get_pte(uint32_t virt)
{
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    uint32_t pde = page_directory[pd_index];
    if (!(pde & PAGE_PRESENT))
        return 0;
    uint32_t *pt = (uint32_t *)(pde & 0xFFFFF000);
    return pt[pt_index];
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

        if (phdr[i].p_vaddr == 0)
        {
            write("Warning: PHDR has p_vaddr == 0, skipping\n");
            continue;
        }

        set_user_pages(phdr[i].p_vaddr, phdr[i].p_memsz);

        unsigned char *dest = (unsigned char *)(phdr[i].p_vaddr);
        unsigned char *src = buf + phdr[i].p_offset;

        for (Elf32_Word j = 0; j < phdr[i].p_filesz; j++)
            dest[j] = src[j];

        for (Elf32_Word j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++)
            dest[j] = 0;
    }

    unsigned int stack_bottom = USER_STACK_TOP - USER_STACK_PAGES * PAGE_SIZE;
    set_user_pages(stack_bottom, USER_STACK_PAGES * PAGE_SIZE);

    write("Jumping to user program in ring3...\n");

    uint32_t pte_entry = get_pte(ehdr->e_entry);
    if ((pte_entry & PAGE_PRESENT) && (pte_entry & PAGE_USER))
        jump_to_user(ehdr->e_entry);
    else
        write("ERROR: entry not mapped as user (abort jump)\n");
}