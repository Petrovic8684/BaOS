#ifndef INFO_H
#define INFO_H

struct utsname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

extern struct utsname uname_info;

#endif
