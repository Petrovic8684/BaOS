#include <stdio.h>
#include <sys_utsname.h>

int main(void)
{
    struct utsname info;
    if (uname(&info) == 0)
        printf("Kernel version: %s\n", info.version);
    else
        printf("\033[31mError: Could not get kernel version.\033[0m\n");

    return 0;
}
