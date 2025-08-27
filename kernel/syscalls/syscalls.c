#include "../drivers/display/display.h"
#include "../helpers/string/string.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_CLEAR 2

#define USER_BUFFER_SIZE 256

void return_to_loader(void)
{
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
        write("User program exited.");
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
        write("Unknown syscall\n");
        break;
    }
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    asm volatile(
        ".intel_syntax noprefix\n\t"
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
