#include <stdio.h>
#include <unistd.h>

#define SYS_POWER_OFF 4

int main(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_POWER_OFF)
        : "eax", "memory");

    return 0;
}
