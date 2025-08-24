#include "wrappers.h"

void wrapper_clear(void)
{
    clear();
}

void wrapper_list_dir(void)
{
    fs_list_dir();
}

void wrapper_make_dir(const char *name)
{
    int r = fs_make_dir(name);

    switch (r)
    {
    case 0:
        write_colored("Directory created successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error while creating directory.\n", 0x04);
        break;
    case FS_ERR_EXISTS:
        write_colored("Error: Directory or file with this name already exists.\n", 0x04);
        break;
    case FS_ERR_NO_SPACE:
        write_colored("Error: No space left for new directory.\n", 0x04);
        break;
    case FS_ERR_NAME_LONG:
        write_colored("Error: Directory name too long.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while creating directory.\n", 0x04);
        break;
    }
}

void wrapper_make_file(const char *name)
{
    int r = fs_make_file(name);

    switch (r)
    {
    case 0:
        write_colored("File created successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error while creating file.\n", 0x04);
        break;
    case FS_ERR_EXISTS:
        write_colored("Error: File or directory with this name already exists.\n", 0x04);
        break;
    case FS_ERR_NO_SPACE:
        write_colored("Error: No space left for new file.\n", 0x04);
        break;
    case FS_ERR_NAME_LONG:
        write_colored("Error: File name too long.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while creating file.\n", 0x04);
        break;
    }
}

void wrapper_change_dir(const char *name)
{
    int res = fs_change_dir(name);

    switch (res)
    {
    case FS_OK:
        write_colored("Changed directory successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error while changing directory.\n", 0x04);
        break;
    case FS_ERR_NO_DRV:
        write_colored("Error: No ATA driver found.\n", 0x04);
        break;
    case FS_ERR_NAME_LONG:
        write_colored("Error: Directory name too long.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    case FS_ERR_NOT_EXISTS:
        write_colored("Error: No such file or directory.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while changing directory.\n", 0x04);
        break;
    }
}

void wrapper_where(void)
{
    fs_where();
}

void wrapper_delete_dir(const char *name)
{
    int res = fs_delete_dir(name);

    switch (res)
    {
    case FS_OK:
        write_colored("Directory deleted successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error while deleting directory.\n", 0x04);
        break;
    case FS_ERR_EXISTS:
        write_colored("Error: Directory is not empty.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    case FS_ERR_NOT_EXISTS:
        write_colored("Error: No such file or directory.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while deleting directory.\n", 0x04);
        break;
    }
}

void wrapper_delete_file(const char *name)
{
    int res = fs_delete_file(name);

    switch (res)
    {
    case FS_OK:
        write_colored("File deleted successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error while deleting file.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    case FS_ERR_NOT_EXISTS:
        write_colored("Error: No such file or directory.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while deleting file.\n", 0x04);
        break;
    }
}

void wrapper_write_file(const char *name, const char *text)
{
    int ret = fs_write_file(name, text);
    switch (ret)
    {
    case FS_OK:
        write_colored("File written successfully.\n", 0x02);
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error occurred while writing to file.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    case FS_ERR_NO_TEXT:
        write_colored("Error: Invalid text provided.\n", 0x04);
        break;
    case FS_ERR_NOT_EXISTS:
        write_colored("Error: No such file or directory.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while writing to file.\n", 0x04);
        break;
    }
}

void wrapper_read_file(const char *name)
{
    int ret = fs_read_file(name);
    switch (ret)
    {
    case FS_OK:
        break;
    case FS_ERR_NOT_INIT:
        write_colored("Error: File system not initialized.\n", 0x04);
        break;
    case FS_ERR_IO:
        write_colored("Error: I/O error occurred while reading file.\n", 0x04);
        break;
    case FS_ERR_NO_NAME:
        write_colored("Error: Invalid name provided.\n", 0x04);
        break;
    case FS_ERR_NOT_EXISTS:
        write_colored("Error: No such file or directory.\n", 0x04);
        break;
    default:
        write_colored("Error: Unknown error while reading file.\n", 0x04);
        break;
    }
}

void wrapper_info(const char *name)
{
    fs_info(name);
}

void wrapper_echo(const char *arg1, const char *arg2)
{
    if ((!arg1 || arg1[0] == 0) && (!arg2 || arg2[0] == 0))
    {
        write_colored("Error: Invalid text provided.\n", 0x04);
        return;
    }

    write(arg1);
    if (arg2 && arg2[0] != 0)
    {
        write(" ");
        write(arg2);
    }

    write("\n");
}

void wrapper_print_date(void)
{
    DateTime dt = rtc_now();
    char buf[5];

    write("Date: ");
    itoa(dt.day, buf);
    write(buf);
    write("-");
    itoa(dt.month, buf);
    write(buf);
    write("-");
    itoa(dt.year, buf);
    write(buf);
    write(" ");
    itoa(dt.hour, buf);
    write(buf);
    write(":");
    itoa(dt.minute, buf);
    write(buf);
    write(":");
    itoa(dt.second, buf);
    write(buf);
    write("\n");
}

void wrapper_help(void)
{
    clear();
    write(
        "Commands:\n"
        "  where        - Show current directory\n"
        "  list         - List files and directories\n"
        "  info         - Show file or directory information\n"
        "  echo         - Print text to the console\n"
        "  date         - Show current date and time\n"
        "  makedir      - Create directory\n"
        "  changedir    - Change directory\n"
        "  deletedir    - Delete directory\n"
        "  makefile     - Create file\n"
        "  deletefile   - Delete file\n"
        "  writefile    - Write text into file\n"
        "  readfile     - Read text from file\n"
        "  osname       - Show OS name\n"
        "  version      - Show kernel version\n"
        "  help         - Show this help message\n");
}

void wrapper_os_name(void)
{
    write("OS name: ");
    write(os_name());
    write("\n");
}

void wrapper_kernel_version(void)
{
    write("Kernel version: ");
    write(kernel_version());
    write("\n");
}

void wrapper_shutdown(void)
{
    write("Shutting down...\n");
    power_off();
    write_colored("Error: Shutdown failed.\n", 0x02);
}
