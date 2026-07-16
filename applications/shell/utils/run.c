#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYS_LOAD_USER_PROGRAM 17

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("\033[1;33mUsage:\033[0m run <program> [args...]\n");
        printf("Load and execute an ELF program via the kernel loader.\n");
        printf("Example: run /programs/out1\n");
        printf("         run /programs/out1 -code\n");
        printf("         run /programs/out1 --code\n");
        return 1;
    }

    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[args], %%ebx\n\t"
        "movl %[prog], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_LOAD_USER_PROGRAM),
          [prog] "r"(argv[1]),
          [args] "r"(&argv[1])
        : "eax", "ebx", "ecx", "memory");

    return 0;
}
