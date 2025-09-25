#include <sys/sysinfo.h>

#define SYS_UPTIME 24

int sysinfo(struct sysinfo *info)
{
    unsigned long ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_UPTIME)
        : "eax", "memory");

    info->uptime = ret;

    return 0;
}