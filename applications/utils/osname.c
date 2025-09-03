#include <stdio.h>
#include <sys_utsname.h>

int main(void)
{
    struct utsname info;
    if (uname(&info) == 0)
        printf("OS name: %s\n", info.sysname);
    else
        printf("\033[31mError: Could not get OS name.\033[0m\n");

    return 0;
}
