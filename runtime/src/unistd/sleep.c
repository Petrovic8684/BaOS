#include <unistd.h>
#include "internal/syscalls.h"

unsigned int sleep(unsigned int seconds)
{
    sys_sleep(seconds * 1000);
    return 0;
}
