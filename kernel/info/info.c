#include "info.h"

#define OS_NAME "BaOS (Jovan Petrovic, 2025)"
#define KERNEL_VERSION "1.1"

const char *os_name(void)
{
    return OS_NAME;
}

const char *kernel_version(void)
{
    return KERNEL_VERSION;
}
