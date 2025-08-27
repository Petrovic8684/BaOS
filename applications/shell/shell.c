#include "shell.h"

void main(void)
{
    char buffer[80];

    while (1)
    {
        printf("BaOS %s> ", fs_get_current_dir_name());
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
    if (strcmp(cmd, "info") == 0)
        return CMD_INFO;
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
    if (strcmp(cmd, "filling") == 0)
        return CMD_FILLING;
    if (strcmp(cmd, "calc") == 0)
        return CMD_CALC;
    return CMD_UNKNOWN;
}

void process_command(char *cmd)
{
    int i = 0, j;
    char command[32], arg1[32], arg2[256];

    j = 0;
    while (cmd[i] && cmd[i] != ' ')
        command[j++] = cmd[i++];
    command[j] = 0;

    while (cmd[i] == ' ')
        i++;
    j = 0;
    while (cmd[i] && cmd[i] != ' ')
        arg1[j++] = cmd[i++];
    arg1[j] = 0;

    while (cmd[i] == ' ')
        i++;
    j = 0;
    while (cmd[i])
        arg2[j++] = cmd[i++];
    arg2[j] = 0;

    Command c = parse_command(command);

    switch (c)
    {
    case CMD_CLEAR:
        wrapper_clear();
        return;
    case CMD_FILLING:
        wrapper_filling(arg1);
        return;
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
    case CMD_INFO:
        wrapper_info(arg1);
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
    case CMD_CALC:
        wrapper_calc(arg1);
        break;
    default:
        printf("Error: Unknown command. Type 'help' for a list of valid commands.\n");
        break;
    }

    printf("\n");
}