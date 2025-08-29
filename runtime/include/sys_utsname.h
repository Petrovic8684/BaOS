#ifndef SYS_UTSNAME_H
#define SYS_UTSNAME_H

struct utsname
{
    char sysname[64];
    char nodename[64];
    char release[32];
    char version[32];
    char machine[32];
};

int uname(struct utsname *buf);

#endif
