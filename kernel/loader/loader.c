#include "loader.h"

#define USER_BUFFER_SIZE 1024
#define MAX_ARGC 64
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_PAGES * PAGE_SIZE)

unsigned int loader_return_eip = 0;
unsigned int loader_saved_esp = 0;
unsigned int loader_saved_ebp = 0;

const char *next_prog_name = 0;
const char **next_prog_argv = 0;

void (*loader_post_return_callback)(void) = 0;

static void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];

    kernel_buf[i] = '\0';
}

static void jump_to_user(unsigned int entry, unsigned int stack)
{
    asm volatile("cli\n\t"
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
                 : [entry] "r"(entry), [stack] "r"(stack)
                 : "ax");
}

static void load_shell_again(void)
{
    write("\n");
    load_user_program("shell.bin", ((void *)0));
}

void load_next_program(void)
{
    if (!next_prog_argv || !next_prog_argv[0])
    {
        write_colored("load_next_program: no next program set\n", 0x04);
        return;
    }

    loader_post_return_callback = load_shell_again;

    load_user_program(next_prog_argv[0], next_prog_argv);

    next_prog_name = 0;
    next_prog_argv = 0;
}

void load_user_program(const char *name, const char **user_argv)
{
    if (!fs_initialized)
    {
        write_colored("FS not initialized.\n", 0x04);
        load_shell_again();

        return;
    }

    unsigned char buf[65536];
    unsigned int size = 0;

    if (fs_read_file_buffer(name, buf, sizeof(buf), &size) != FS_OK)
    {
        write_colored("Failed to read user program.\n", 0x04);
        load_shell_again();

        return;
    }

    if (size == 0)
    {
        write("User program empty.\n");
        load_shell_again();

        return;
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
    {
        write_colored("Not a valid ELF file.\n", 0x04);
        load_shell_again();

        return;
    }

    Elf32_Phdr *phdr = (Elf32_Phdr *)(buf + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        if (phdr[i].p_vaddr == 0)
        {
            write_colored("Warning: PHDR has p_vaddr == 0, skipping\n", 0x04);
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

    set_user_pages(USER_STACK_BOTTOM, USER_STACK_PAGES * PAGE_SIZE);

    char kernel_buf[USER_BUFFER_SIZE];
    char *string_ptrs[MAX_ARGC];
    int argc = 0;
    unsigned int stack_top = USER_STACK_TOP;
    unsigned int cur = stack_top;

    for (int i = 0; i < MAX_ARGC; ++i)
    {
        if (!user_argv || user_argv[i] == ((void *)0))
            break;

        copy_from_user(kernel_buf, user_argv[i], USER_BUFFER_SIZE);

        unsigned int slen = 0;
        while (slen < USER_BUFFER_SIZE && kernel_buf[slen])
            slen++;
        unsigned int needed = slen + 1;

        if (cur < USER_STACK_BOTTOM + needed)
        {
            write_colored("Not enough user stack space for args.\n", 0x04);
            load_shell_again();

            return;
        }

        cur -= needed;
        char *dest = (char *)cur;
        for (unsigned int k = 0; k <= slen; ++k)
            dest[k] = kernel_buf[k];

        string_ptrs[argc] = dest;
        argc++;
    }

    cur &= ~0x3u;

    unsigned int argv_array_addr = cur - ((argc + 1) * sizeof(char *));
    if (argv_array_addr < USER_STACK_BOTTOM)
    {
        write_colored("Not enough user stack space for argv array.\n", 0x04);
        load_shell_again();

        return;
    }

    for (int i = 0; i < argc; ++i)
        ((unsigned int *)(argv_array_addr))[i] = (unsigned int)string_ptrs[i];

    ((unsigned int *)(argv_array_addr))[argc] = 0;

    unsigned int final_stack = argv_array_addr - 2 * sizeof(unsigned int);
    if (final_stack < USER_STACK_BOTTOM)
    {
        write_colored("Not enough user stack space for argc/argv ptr.\n", 0x04);
        load_shell_again();

        return;
    }

    ((unsigned int *)final_stack)[0] = argc;
    ((unsigned int *)final_stack)[1] = argv_array_addr;

    loader_return_eip = (unsigned int)&&user_return;
    asm volatile("mov %%esp, %0" : "=r"(loader_saved_esp));
    asm volatile("mov %%ebp, %0" : "=r"(loader_saved_ebp));

    tss_set_esp0(loader_saved_esp);

    unsigned int pte_entry = get_pte(ehdr->e_entry);
    if ((pte_entry & PAGE_PRESENT) && (pte_entry & PAGE_USER))
        jump_to_user(ehdr->e_entry, final_stack);
    else
    {
        write_colored("Entry not mapped as user (abort jump).\n", 0x04);
        load_shell_again();

        return;
    }

user_return:
    loader_return_eip = 0;
    loader_saved_esp = 0;
    loader_saved_ebp = 0;

    tss_set_esp0(0x00090000);

    if (loader_post_return_callback)
        loader_post_return_callback();

    loader_post_return_callback = 0;
    return;
}