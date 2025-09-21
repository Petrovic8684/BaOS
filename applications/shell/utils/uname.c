#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
    struct utsname info;
    if (uname(&info) == 0)
    {
        printf("\033[1;33mOS name:\033[0m %s\n", info.sysname);
        printf("\033[1;33mNode name:\033[0m %s\n", info.nodename);
        printf("\033[1;33mRelease:\033[0m %s\n", info.release);
        printf("\033[1;33mVersion:\033[0m %s\n", info.version);
        printf("\033[1;33mMachine:\033[0m %s\n", info.machine);
    }
    else
        printf("\033[31mError: Could not retrieve system info.\033[0m\n");

    return 0;
}
