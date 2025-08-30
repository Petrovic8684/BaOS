#include "shell.h"
#include "./wrappers/wrappers.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void main(void)
{
    char buffer[80];

    while (true)
    {
        printf("BaOS %s> ", wrapper_get_current_dir_name());
        read_line(buffer, 80);
        printf("\n");
        process_command(buffer);
    }
}

Command parse_command(const char *cmd)
{
    if (strcmp(cmd, "clear") == 0)
        return CMD_CLEAR;
    if (strcmp(cmd, "list") == 0)
        return CMD_LIST;
    if (strcmp(cmd, "makedir") == 0)
        return CMD_MAKEDIR;
    if (strcmp(cmd, "makefile") == 0)
        return CMD_MAKEFILE;
    if (strcmp(cmd, "changedir") == 0)
        return CMD_CHANGEDIR;
    if (strcmp(cmd, "where") == 0)
        return CMD_WHERE;
    if (strcmp(cmd, "echo") == 0)
        return CMD_ECHO;
    if (strcmp(cmd, "date") == 0)
        return CMD_DATE;
    if (strcmp(cmd, "deletedir") == 0)
        return CMD_DELETEDIR;
    if (strcmp(cmd, "deletefile") == 0)
        return CMD_DELETEFILE;
    if (strcmp(cmd, "writefile") == 0)
        return CMD_WRITEFILE;
    if (strcmp(cmd, "readfile") == 0)
        return CMD_READFILE;
    if (strcmp(cmd, "help") == 0)
        return CMD_HELP;
    if (strcmp(cmd, "osname") == 0)
        return CMD_OSNAME;
    if (strcmp(cmd, "version") == 0)
        return CMD_KERNELVERSION;
    if (strcmp(cmd, "shutdown") == 0)
        return CMD_SHUTDOWN;
    if (strcmp(cmd, "run") == 0)
        return CMD_RUN;
    return CMD_UNKNOWN;
}

void process_command(char *cmd)
{
    char command[32] = {0};
    char arg1[32] = {0};
    char arg2[256] = {0};

    char *token = strtok(cmd, " ");
    if (token)
        strncpy(command, token, sizeof(command) - 1);

    token = strtok(NULL, " ");
    if (token)
        strncpy(arg1, token, sizeof(arg1) - 1);

    token = strtok(NULL, "");
    if (token)
        strncpy(arg2, token, sizeof(arg2) - 1);

    Command c = parse_command(command);

    switch (c)
    {
    case CMD_CLEAR:
        wrapper_clear();
        break;
    case CMD_LIST:
        wrapper_list_dir();
        break;
    case CMD_MAKEDIR:
        wrapper_make_dir(arg1);
        break;
    case CMD_MAKEFILE:
        wrapper_make_file(arg1);
        break;
    case CMD_CHANGEDIR:
        wrapper_change_dir(arg1);
        break;
    case CMD_WHERE:
        wrapper_where();
        break;
    case CMD_ECHO:
        wrapper_echo(arg1, arg2);
        break;
    case CMD_DATE:
        wrapper_print_date();
        break;
    case CMD_DELETEDIR:
        wrapper_delete_dir(arg1);
        break;
    case CMD_DELETEFILE:
        wrapper_delete_file(arg1);
        break;
    case CMD_WRITEFILE:
        wrapper_write_file(arg1, arg2);
        break;
    case CMD_READFILE:
        wrapper_read_file(arg1);
        break;
    case CMD_HELP:
        wrapper_help();
        break;
    case CMD_OSNAME:
        wrapper_os_name();
        break;
    case CMD_KERNELVERSION:
        wrapper_kernel_version();
        break;
    case CMD_SHUTDOWN:
        wrapper_shutdown();
        break;
    case CMD_RUN:
        wrapper_run(arg1, arg2);
        break;
    default:
        printf("\033[31mError: Unknown command. Type 'help' for a list of valid commands.\033[0m\n");
        break;
    }

    printf("\n");
}