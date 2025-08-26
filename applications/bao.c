#include "./include/user_syscalls.h"

void _start()
{
    write("Hello from ring3!");
    while (1)
        ;
}
