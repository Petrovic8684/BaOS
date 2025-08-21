#ifndef SYSTEM_H
#define SYSTEM_H

#include "../util/util.h"
#include "../ports/ports.h"

typedef struct
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
} DateTime;

DateTime date(void);
const char *os_name(void);
const char *kernel_version(void);

#endif
