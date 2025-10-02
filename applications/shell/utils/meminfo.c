#include <stdio.h>

#define SYS_HEAP_INFO 22

struct heap_info
{
    unsigned int heap_start;
    unsigned int heap_end;
    unsigned int heap_max;
    unsigned int free_bytes;
};

void print_human(unsigned int bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    double size = (double)bytes;

    int i = 0;
    while (size >= 1024.0 && i < 3)
    {
        size /= 1024.0;
        i++;
    }

    printf("%f %s", size, units[i]);
}

int main(void)
{
    struct heap_info hi;

    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_HEAP_INFO), [arg] "r"(&hi)
        : "eax", "ebx", "memory");

    unsigned int remaining = hi.heap_max - hi.heap_end;
    unsigned int used = (hi.heap_end - hi.heap_start) - hi.free_bytes;

    printf("\033[1;33m--- HEAP INFO ---\033[0m\n\n");

    printf("\033[1;33mStart:\033[0m  0x%x\n", hi.heap_start);
    printf("\033[1;33mEnd:\033[0m    0x%x\n", hi.heap_end);
    printf("\033[1;33mMax:\033[0m    0x%x\n\n", hi.heap_max);

    printf("\033[1;33mActually used:\033[0m  ");
    print_human(used);
    printf("\n");

    printf("\033[1;33mCurrently free:\033[0m ");
    print_human(hi.free_bytes);
    printf("\n");

    printf("\033[1;33mAvailable:\033[0m      ");
    print_human(remaining);
    printf("\n");

    return 0;
}
