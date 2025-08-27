#include "../drivers/display/display.h"
#include "../helpers/string/string.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_CLEAR 2

#define USER_BUFFER_SIZE 256

extern unsigned int loader_return_eip;
extern unsigned int loader_saved_esp;
extern unsigned int loader_saved_ebp;

__attribute__((naked)) void return_to_loader(void)
{
    asm volatile(".intel_syntax noprefix\n\t"
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

void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];

    kernel_buf[i] = '\0';
}

void handle_syscall(unsigned int num, unsigned int arg)
{
    switch (num)
    {
    case SYS_EXIT:
        return_to_loader();
        break;
    case SYS_WRITE:
    {
        char buffer[USER_BUFFER_SIZE];
        copy_from_user(buffer, (const char *)arg, sizeof(buffer));
        write(buffer);
        break;
    }
    case SYS_CLEAR:
    {
        clear();
        break;
    }
    default:
        write_colored("Unknown syscall.\n", 0x04);
        break;
    }
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    asm volatile(".intel_syntax noprefix\n\t"
                 "cli\n\t"
                 "push ebp\n\t"
                 "mov ebp, esp\n\t"
                 "push ebx\n\t"
                 "push eax\n\t"
                 "call handle_syscall\n\t"
                 "add esp, 8\n\t"
                 "pop ebp\n\t"
                 "sti\n\t"
                 "iret\n\t"
                 ".att_syntax\n\t");
}