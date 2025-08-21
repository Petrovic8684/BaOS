#include "wrappers.h"

void wrapper_clear(void) { clear(); }

void wrapper_list_dir(void) { fs_list_dir(); }

void wrapper_make_dir(const char *name) { fs_make_dir(name); }

void wrapper_make_file(const char *name) { fs_make_file(name); }

void wrapper_change_dir(const char *name) { fs_change_dir(name); }

void wrapper_where(void) { fs_where(); }

void wrapper_delete_dir(const char *name) { fs_delete_dir(name); }

void wrapper_delete_file(const char *name) { fs_delete_file(name); }

void wrapper_write_file(const char *name, const char *text) { fs_write_file(name, text); }

void wrapper_info(const char *name) { fs_info(name); }

void wrapper_read_file(const char *name) { fs_read_file(name); }

void wrapper_echo(const char *arg1, const char *arg2)
{
    if (arg1[0] == 0 && arg2[0] == 0)
    {
        write("Text cannot be empty!\n");
        return;
    }
    write(arg1);
    if (arg2[0] != 0)
    {
        write(" ");
        write(arg2);
    }
    write("\n");
}

void wrapper_print_date(void)
{
    DateTime dt = date();

    char buf[4];
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
    write("Commands:\n"
          "    where        - Show current directory\n"
          "    list         - List files and directories\n"
          "    info         - Show file or directory information\n"
          "    echo         - Print text to the console\n"
          "    date         - Show current date and time\n"
          "    makedir      - Create directory\n"
          "    changedir    - Change directory\n"
          "    deletedir    - Delete directory\n"
          "    makefile     - Create file\n"
          "    deletefile   - Delete file\n"
          "    writefile    - Write text into file\n"
          "    readfile     - Read text from file\n"
          "    osname       - Show OS name\n"
          "    version      - Show kernel version\n"
          "    help         - Show this help message\n");
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