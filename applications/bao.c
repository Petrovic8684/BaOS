#include "./include/user_syscalls.h"

void _start()
{
    write("Hello from ring3!\n"); // ovo se ne ispisuje na ekranu
    while (1)
        ;
}
