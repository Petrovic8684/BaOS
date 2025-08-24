#ifndef SHELL_H
#define SHELL_H

#include "./history/history.h"
#include "./wrappers/wrappers.h"

typedef enum
{
    CMD_UNKNOWN,
    CMD_CLEAR,
    CMD_LIST,
    CMD_MAKEDIR,
    CMD_MAKEFILE,
    CMD_CHANGEDIR,
    CMD_WHERE,
    CMD_ECHO,
    CMD_DATE,
    CMD_DELETEDIR,
    CMD_DELETEFILE,
    CMD_WRITEFILE,
    CMD_INFO,
    CMD_READFILE,
    CMD_HELP,
    CMD_OSNAME,
    CMD_KERNELVERSION,
    CMD_SHUTDOWN,
    CMD_FILLING
} Command;

void shell_main(void);
Command parse_command(const char *cmd);
void process_command(char *cmd);

#endif
