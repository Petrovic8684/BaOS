#include <sys/sysinfo.h>
#include "internal/syscalls.h"

int sysinfo(struct sysinfo *info)
{
    unsigned long ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_UPTIME)
        : "eax", "ebx", "memory");

    info->uptime = ret;

    return 0;
}
