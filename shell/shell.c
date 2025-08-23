#include "shell.h"

void shell_main(void)
{
    char buffer[80];
    while (1)
    {
        write("BaOS ");
        write(fs_get_current_dir_name());
        write("> ");
        read(buffer, 80, 7 + str_count(fs_get_current_dir_name()));
        write("\n");
        process_command(buffer);
    }
}

Command parse_command(const char *cmd)
{
    if (str_equal(cmd, "clear"))
        return CMD_CLEAR;
    if (str_equal(cmd, "list"))
        return CMD_LIST;
    if (str_equal(cmd, "makedir"))
        return CMD_MAKEDIR;
    if (str_equal(cmd, "makefile"))
        return CMD_MAKEFILE;
    if (str_equal(cmd, "changedir"))
        return CMD_CHANGEDIR;
    if (str_equal(cmd, "where"))
        return CMD_WHERE;
    if (str_equal(cmd, "echo"))
        return CMD_ECHO;
    if (str_equal(cmd, "date"))
        return CMD_DATE;
    if (str_equal(cmd, "deletedir"))
        return CMD_DELETEDIR;
    if (str_equal(cmd, "deletefile"))
        return CMD_DELETEFILE;
    if (str_equal(cmd, "writefile"))
        return CMD_WRITEFILE;
    if (str_equal(cmd, "info"))
        return CMD_INFO;
    if (str_equal(cmd, "readfile"))
        return CMD_READFILE;
    if (str_equal(cmd, "help"))
        return CMD_HELP;
    if (str_equal(cmd, "osname"))
        return CMD_OSNAME;
    if (str_equal(cmd, "version"))
        return CMD_KERNELVERSION;
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
    default:
        write_colored("Error: Unknown command. Type 'help' for a list of valid commands.\n", 0x04);
        break;
    }

    write("\n");
}