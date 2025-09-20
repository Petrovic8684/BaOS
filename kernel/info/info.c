#include "info.h"

#define OS_NAME "BaOS (Jovan Petrovic, 2025)"
#define KERNEL_VERSION "v2.0.0 codename 'Dumpling'"

const char *os_name(void)
{
    return OS_NAME;
}

const char *kernel_version(void)
{
    return KERNEL_VERSION;
}
