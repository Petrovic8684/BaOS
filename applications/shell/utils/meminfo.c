#include <stdio.h>
#include <stdlib.h>

#define SYS_HEAP_INFO 20

static void print_human(unsigned int bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    double size = (double)bytes;

    int i = 0;
    while (size >= 1024.0 && i < 3)
    {
        size /= 1024.0;
        i++;
    }

    printf("%.2f %s", size, units[i]);
}

static void print_heap_section(const char *title, const user_heap_info_t *hi)
{
    unsigned int remaining = hi->heap_max - hi->heap_end;
    unsigned int used = (hi->heap_end - hi->heap_start) - hi->free_bytes;

    printf("\033[1;33m--- %s ---\033[0m\n\n", title);

    printf("\033[1;33mStart:\033[0m  0x%x\n", hi->heap_start);
    printf("\033[1;33mEnd:\033[0m    0x%x\n", hi->heap_end);
    printf("\033[1;33mMax:\033[0m    0x%x\n\n", hi->heap_max);

    printf("\033[1;33mActually used:\033[0m  ");
    print_human(used);
    printf("\n");

    printf("\033[1;33mCurrently free:\033[0m ");
    print_human(hi->free_bytes);
    printf("\n");

    printf("\033[1;33mAvailable:\033[0m      ");
    print_human(remaining);
    printf("\n\n");
}

static int get_kernel_heap_info(user_heap_info_t *info)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_HEAP_INFO), [arg] "r"(info)
        : "eax", "ebx", "memory");

    return ret;
}

int main(void)
{
    user_heap_info_t user_hi;
    user_heap_info_t kernel_hi;

    get_user_heap_info(&user_hi);
    print_heap_section("User program heap", &user_hi);

    if (get_kernel_heap_info(&kernel_hi) == 0)
        print_heap_section("Kernel heap", &kernel_hi);
    else
        printf("\033[31mError: Failed to retrieve kernel heap info.\033[0m\n");

    return 0;
}
