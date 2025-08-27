#include "./user_syscalls.h"

extern int main(int argc, char **argv);

void _start()
{
    int ret = main(0, ((void *)0));

    sys_exit(ret);
}