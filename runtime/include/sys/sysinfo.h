#ifndef SYS_INFO_H
#define SYS_INFO_H

struct sysinfo
{
    unsigned long uptime;
};

int sysinfo(struct sysinfo *info);

#endif