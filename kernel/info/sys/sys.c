#include "sys.h"

#define OS_NAME "BaOS (Jovan Petrovic, 2025)\0"
#define NODENAME "localhost\0"
#define KERNEL_RELEASE "v2.0.0\0"
#define KERNEL_VERSION "Version 2.0.0 (stable), codename 'Dumpling'\0"
#define MACHINE "i386\0"

struct utsname uname_info = {
    .sysname = OS_NAME,
    .nodename = NODENAME,
    .release = KERNEL_RELEASE,
    .version = KERNEL_VERSION,
    .machine = MACHINE};
