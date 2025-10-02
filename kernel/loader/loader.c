#include "loader.h"
#include "../drivers/display/display.h"
#include "../helpers/string/string.h"
#include "../helpers/memory/memory.h"
#include "../fs/fs.h"
#include "../paging/paging.h"
#include "../paging/heap/heap.h"
#include "../system/tss/tss.h"
#include "../info/sys/sys.h"

#define PT_LOAD 1
#define USER_STACK_TOP 0x02100000
#define USER_STACK_PAGES 4
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_PAGES * PAGE_SIZE)

void (*loader_post_return_callback)(void) = 0;

static unsigned int loader_return_eip = 0;
static unsigned int loader_saved_esp = 0;
static unsigned int loader_saved_ebp = 0;

static const char *next_prog_name = 0;
static const char **next_prog_argv = 0;

static unsigned int last_user_region_start = 0;
static unsigned int last_user_region_size = 0;

static void jump_to_user(unsigned int entry, unsigned int stack)
{
    __asm__ volatile("cli\n\t"
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

__attribute__((naked)) void return_to_loader(void)
{
    __asm__ volatile(".intel_syntax noprefix\n\t"
                     "mov ax, 0x10\n\t"
                     "mov ds, ax\n\t"
                     "mov es, ax\n\t"
                     "mov eax, dword ptr [loader_return_eip]\n\t"
                     "test eax, eax\n\t"
                     "jz 1f\n\t"
                     "mov esp, dword ptr [loader_saved_esp]\n\t"
                     "mov ebp, dword ptr [loader_saved_ebp]\n\t"
                     "jmp eax\n\t"
                     "1:\n\t"
                     "cli\n\t"
                     "hlt\n\t"
                     ".att_syntax\n\t");
}

static void cleanup_previous_user_space(void)
{
    unmap_all_user_pages();

    last_user_region_start = 0;
    last_user_region_size = 0;
}

void reset_loader_context(void)
{
    loader_return_eip = 0;
    loader_saved_esp = 0;
    loader_saved_ebp = 0;
}

void set_next_program(const char **argv)
{
    next_prog_argv = argv;
    next_prog_name = argv[0];
}

int load_user_program(const char *name, const char **user_argv, int surpress_errors)
{
    if (!fs_is_initialized())
    {
        if (surpress_errors == 0)
            write("\033[31mError: FS not initialized.\n\033[0m");
        return -1;
    }

    cleanup_previous_user_space();

    unsigned int file_size = 0;
    if (fs_read_file(name, ((void *)0), 0, &file_size) != FS_OK)
    {
        write("\033[31mError: Could not get file size.\033[0m\n");
        return -1;
    }

    unsigned char *buf = kmalloc(file_size);
    unsigned int size = 0;

    if (fs_read_file(name, buf, file_size, &size) != FS_OK)
    {
        if (surpress_errors == 0)
            write("\033[31mError: Failed to read user program.\n\033[0m");

        kfree(buf);
        return -1;
    }

    if (size == 0)
    {
        if (surpress_errors == 0)
            write("User program empty.\n");

        kfree(buf);
        return -1;
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
    {
        if (surpress_errors == 0)
            write("\033[31mError: Not a valid ELF file.\n\033[0m");

        kfree(buf);
        return -1;
    }

    Elf32_Phdr *phdr = (Elf32_Phdr *)(buf + ehdr->e_phoff);

    unsigned int map_min = 0xFFFFFFFFu;
    unsigned int map_max = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        if (phdr[i].p_vaddr == 0)
        {
            if (surpress_errors == 0)
                write("\033[Error: PHDR has p_vaddr == 0, skipping.\n\033[0m");
            continue;
        }

        if (phdr[i].p_vaddr < map_min)
            map_min = phdr[i].p_vaddr;
        unsigned int seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
        if (seg_end > map_max)
            map_max = seg_end;

        (void)set_user_pages(phdr[i].p_vaddr, phdr[i].p_memsz);

        unsigned char *dest = (unsigned char *)(phdr[i].p_vaddr);
        unsigned char *src = buf + phdr[i].p_offset;

        for (Elf32_Word j = 0; j < phdr[i].p_filesz; j++)
            dest[j] = src[j];

        for (Elf32_Word j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++)
            dest[j] = 0;
    }

    kfree(buf);

    if (map_max > map_min && map_min != 0xFFFFFFFFu)
    {
        unsigned int aligned_start = map_min & 0xFFFFF000u;
        unsigned int aligned_end = (map_max + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        last_user_region_start = aligned_start;
        last_user_region_size = aligned_end - aligned_start;
    }
    else
    {
        last_user_region_start = 0;
        last_user_region_size = 0;
    }

    (void)set_user_pages(USER_STACK_BOTTOM, USER_STACK_PAGES * PAGE_SIZE);

    char *string_ptrs[MAX_ARGC];
    char kernel_buf[MAX_ARGV_LEN];
    int argc = 0;
    unsigned int stack_top = USER_STACK_TOP;
    unsigned int cur = stack_top;

    for (int i = 0; i < MAX_ARGC; ++i)
    {
        if (!user_argv || user_argv[i] == ((void *)0))
            break;

        mem_copy(kernel_buf, user_argv[i], str_count(user_argv[i]) + 1);

        unsigned int slen = str_count(kernel_buf);
        unsigned int needed = slen + 1;

        if (cur < USER_STACK_BOTTOM + needed)
        {
            if (surpress_errors == 0)
                write("\033[31mError: Not enough user stack space for args.\n\033[0m");
            return -1;
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
        if (surpress_errors == 0)
            write("\033[31mError: Not enough user stack space for argv array.\n\033[0m");
        return -1;
    }

    for (int i = 0; i < argc; ++i)
        ((unsigned int *)(argv_array_addr))[i] = (unsigned int)string_ptrs[i];

    ((unsigned int *)(argv_array_addr))[argc] = 0;

    unsigned int final_stack = argv_array_addr - 2 * sizeof(unsigned int);
    if (final_stack < USER_STACK_BOTTOM)
    {
        if (surpress_errors == 0)
            write("\033[31mError: Not enough user stack space for argc/argv ptr.\n\033[0m");
        return -1;
    }

    ((unsigned int *)final_stack)[0] = argc;
    ((unsigned int *)final_stack)[1] = argv_array_addr;

    loader_return_eip = (unsigned int)&&user_return;
    __asm__ volatile("mov %%esp, %0" : "=r"(loader_saved_esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(loader_saved_ebp));

    unsigned int pte_entry = get_pte(ehdr->e_entry);
    if ((pte_entry & PAGE_PRESENT) && (pte_entry & PAGE_USER))
        jump_to_user(ehdr->e_entry, final_stack);
    else
    {
        if (surpress_errors == 0)
            write("\033[31mError: Entry not mapped as user (abort jump).\n\033[0m");
        return -1;
    }

user_return:
    reset_loader_context();

    cleanup_previous_user_space();

    void (*cb)(void) = loader_post_return_callback;
    loader_post_return_callback = 0;

    if (cb)
        cb();
    else
        load_shell();

    return 0;
}

void load_next_program(void)
{
    if (!next_prog_argv || !next_prog_argv[0])
    {
        write("\033[31mError: No next program set.\033[0m\n");
        return;
    }

    loader_post_return_callback = load_shell;

    const char *prog = next_prog_argv[0];
    const char **argv = next_prog_argv;

    next_prog_name = 0;
    next_prog_argv = 0;

    load_user_program(prog, argv, 0);
}

static void welcome_callback(void)
{
    write("      Welcome to BaOS - the Bourne again Operating System!\n      Type '\033[1;32mhelp\033[0m' to see a list of available commands.\n\n      ");
    write(uname_info.version);
    write(".\n\n");

    load_shell();
}

void load_welcome(void)
{
    write("\n");
    loader_post_return_callback = welcome_callback;
    if (load_user_program("/programs/utils/banner", (const char *[]){"banner", " BaOS", "-color=lightgreen", ((void *)0)}, 1) == 0)
        return;

    clear();
    write("\n\033[1;33mWarning: Could not load welcome program.\033[0m\n");
}

void load_shell(void)
{
    int tries = 0;

    for (;;)
    {
        write("\n");

        if (load_user_program("/programs/shell", ((void *)0), 1) == 0)
        {
            tries = 0;
            continue;
        }

        tries++;
        write("\033[31mError: Failed to load the shell (attempt ");
        write_dec(tries);
        write(").\033[0m");

        if (tries >= 3)
        {
            write("\n\n\033[31mFailed to load the shell after 3 attempts. Halting...\033[0m\n");
            for (;;)
                __asm__ volatile("hlt");
        }
    }
}
