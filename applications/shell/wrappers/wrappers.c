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
        printf("Directory created successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error while creating directory.\n");
        break;
    case FS_ERR_EXISTS:
        printf("Error: Directory or file with this name already exists.\n");
        break;
    case FS_ERR_NO_SPACE:
        printf("Error: No space left for new directory.\n");
        break;
    case FS_ERR_NAME_LONG:
        printf("Error: Directory name too long.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    default:
        printf("Error: Unknown error while creating directory.\n");
        break;
    }
}

void wrapper_make_file(const char *name)
{
    int r = fs_make_file(name);

    switch (r)
    {
    case 0:
        printf("File created successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error while creating file.\n");
        break;
    case FS_ERR_EXISTS:
        printf("Error: File or directory with this name already exists.\n");
        break;
    case FS_ERR_NO_SPACE:
        printf("Error: No space left for new file.\n");
        break;
    case FS_ERR_NAME_LONG:
        printf("Error: File name too long.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    default:
        printf("Error: Unknown error while creating file.\n");
        break;
    }
}

void wrapper_change_dir(const char *name)
{
    int res = fs_change_dir(name);

    switch (res)
    {
    case FS_OK:
        printf("Changed directory successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error while changing directory.\n");
        break;
    case FS_ERR_NO_DRV:
        printf("Error: No ATA driver found.\n");
        break;
    case FS_ERR_NAME_LONG:
        printf("Error: Directory name too long.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    case FS_ERR_NOT_EXISTS:
        printf("Error: No such file or directory.\n");
        break;
    default:
        printf("Error: Unknown error while changing directory.\n");
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
        printf("Directory deleted successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error while deleting directory.\n");
        break;
    case FS_ERR_EXISTS:
        printf("Error: Directory is not empty.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    case FS_ERR_NOT_EXISTS:
        printf("Error: No such file or directory.\n");
        break;
    default:
        printf("Error: Unknown error while deleting directory.\n");
        break;
    }
}

void wrapper_delete_file(const char *name)
{
    int res = fs_delete_file(name);

    switch (res)
    {
    case FS_OK:
        printf("File deleted successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error while deleting file.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    case FS_ERR_NOT_EXISTS:
        printf("Error: No such file or directory.\n");
        break;
    default:
        printf("Error: Unknown error while deleting file.\n");
        break;
    }
}

void wrapper_write_file(const char *name, const char *text)
{
    int ret = fs_write_file(name, text);
    switch (ret)
    {
    case FS_OK:
        printf("File written successfully.\n");
        break;
    case FS_ERR_NOT_INIT:
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error occurred while writing to file.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    case FS_ERR_NO_TEXT:
        printf("Error: Invalid text provided.\n");
        break;
    case FS_ERR_NOT_EXISTS:
        printf("Error: No such file or directory.\n");
        break;
    default:
        printf("Error: Unknown error while writing to file.\n");
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
        printf("Error: File system not initialized.\n");
        break;
    case FS_ERR_IO:
        printf("Error: I/O error occurred while reading file.\n");
        break;
    case FS_ERR_NO_NAME:
        printf("Error: Invalid name provided.\n");
        break;
    case FS_ERR_NOT_EXISTS:
        printf("Error: No such file or directory.\n");
        break;
    default:
        printf("Error: Unknown error while reading file.\n");
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
        printf("Error: Invalid text provided.\n");
        return;
    }

    printf(arg1);
    if (arg2 && arg2[0] != 0)
    {
        printf(" ");
        printf(arg2);
    }

    printf("\n");
}

void wrapper_print_date(void)
{
    rtc_now();
}

void wrapper_help(void)
{
    wrapper_clear();
    printf(
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
        "  calc         - A powerful calculator utility\n"
        "  filling      - A simple text editor\n"
        "  osname       - Show OS name\n"
        "  version      - Show kernel version\n"
        "  help         - Show this help message\n");
}

void wrapper_os_name(void)
{
    printf("OS name: %s\n", os_name());
}

void wrapper_kernel_version(void)
{
    printf("Kernel version: %s\n", kernel_version());
}

void wrapper_shutdown(void)
{
    power_off();
}

void wrapper_filling(const char *name)
{
    load_user_program("filling");
}

void wrapper_calc(const char *expr)
{
    load_user_program("calc");
}