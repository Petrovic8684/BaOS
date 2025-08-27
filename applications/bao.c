#include "./include/user_syscalls.h"

void _start()
{
    clear();
    write("Hello from ring3!");

    // exit(0);

    while (1)
        ;
}
