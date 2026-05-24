#include <unistd.h>
#include "internal/syscalls.h"

int usleep(useconds_t usec)
{
    unsigned int ms = (usec + 999) / 1000;
    sys_sleep(ms);
    return 0;
}
