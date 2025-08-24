#ifndef SYSTEM_H
#define SYSTEM_H

#include "./acpi/acpi.h"

#define OS_NAME "BaOS (Jovan Petrovic, 2025)"
#define KERNEL_VERSION "1.1"

void power_off(void);
const char *os_name(void);
const char *kernel_version(void);

#endif
