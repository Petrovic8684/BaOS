#include "wrappers.h"

void wrapper_clear(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_CLEAR)
        : "eax", "memory");
}

void wrapper_list_dir(void)
{
    DIR *d = opendir(NULL);
    if (!d)
    {
        printf("Error: Could not open current directory for listing.\n");
        return;
    }

    struct dirent *ent;
    int printed_any = 0;
    while ((ent = readdir(d)) != NULL)
    {
        printf("%s ", ent->d_name);
        printed_any = 1;
    }

    if (!printed_any)
        printf("(empty directory)");

    printf("\n");
    closedir(d);
}

void wrapper_make_dir(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("Error: Invalid name provided.\n");
        return;
    }

    if (mkdir(name, 0755) == 0)
    {
        printf("Directory created successfully.\n");
        return;
    }
}

void wrapper_make_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("Error: Invalid name provided.\n");
        return;
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("Error: Could not create file '%s'.\n", name);
        return;
    }
    if (fclose(f) != 0)
    {
        printf("Error: Failed to finalize file '%s'.\n", name);
        return;
    }

    printf("File created successfully.\n");
}

static const char *where(void)
{
    static char buf[256];
    if (getcwd(buf, sizeof(buf)))
        ;
    else
    {
        buf[0] = '?';
        buf[1] = '\0';
    }

    return buf;
}

void wrapper_where(void)
{
    const char *path = where();
    printf("%s\n", path);
}

const char *wrapper_get_current_dir_name(void)
{
    static char last_dir[128];
    const char *path = where();
    if (!path)
        return "/";

    size_t len = strlen(path);

    if (len == 1 && path[0] == '/')
    {
        last_dir[0] = '/';
        last_dir[1] = '\0';
        return last_dir;
    }

    const char *last_slash = path + len - 1;
    while (last_slash > path && *last_slash == '/')
        last_slash--;

    const char *prev_slash = last_slash;
    while (prev_slash > path && *prev_slash != '/')
        prev_slash--;

    size_t seg_len = last_slash - prev_slash;
    if (*prev_slash == '/')
        prev_slash++;

    if (seg_len >= sizeof(last_dir))
        seg_len = sizeof(last_dir) - 1;

    for (size_t i = 0; i < seg_len; i++)
        last_dir[i] = prev_slash[i];
    last_dir[seg_len] = '\0';

    return last_dir;
}

void wrapper_change_dir(const char *name)
{
    if (chdir(name) == 0)
        printf("Changed directory successfully.\n");
    else
        printf("Error: could not change directory to '%s'.\n", name);
}

void wrapper_delete_dir(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("Error: Invalid name provided.\n");
        return;
    }

    int ret = rmdir(name);
    if (ret == 0)
    {
        printf("Directory deleted successfully.\n");
        return;
    }
}

void wrapper_delete_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("Error: Invalid name provided.\n");
        return;
    }

    int r = remove(name);

    if (r == 0)
        printf("File deleted successfully.\n");
    else
        printf("Error: Could not delete file '%s'.\n", name);
}

void wrapper_write_file(const char *name, const char *text)
{
    if (!name || !text)
    {
        printf("Error: Invalid name or text provided.\n");
        return;
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("Error: Could not open file '%s' for writing.\n", name);
        return;
    }

    size_t len = 0;
    while (text[len])
        len++;

    size_t written = fwrite(text, 1, len, f);
    if (written != len)
    {
        printf("Error: I/O error occurred while writing to file '%s'.\n", name);
        fclose(f);
        return;
    }

    if (fflush(f) == EOF)
    {
        printf("Error: I/O error occurred while writing to file '%s'.\n", name);
        fclose(f);
        return;
    }

    if (fclose(f) != 0)
    {
        printf("Error: Failed to close file '%s' after writing.\n", name);
        return;
    }

    printf("File written successfully.\n");
}

void wrapper_read_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("Error: Invalid name provided.\n");
        return;
    }

    FILE *f = fopen(name, "r");
    if (!f)
    {
        printf("Error: Could not open file '%s' for reading.\n", name);
        return;
    }

    unsigned char buffer[256];
    size_t n;
    int any = 0;

    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        fwrite(buffer, 1, n, stdout);
        any = 1;
    }

    if (ferror(f))
    {
        printf("\nError: I/O error occurred while reading file '%s'.\n", name);
        fclose(f);
        return;
    }

    if (any)
        printf("\n");

    fclose(f);
}

void wrapper_echo(const char *arg1, const char *arg2)
{
    if ((!arg1 || arg1[0] == 0) && (!arg2 || arg2[0] == 0))
    {
        printf("Error: Invalid text provided.\n");
        return;
    }

    if (arg1 && arg1[0] != 0)
        printf("%s", arg1);
    if (arg2 && arg2[0] != 0)
        printf(" %s", arg2);
    printf("\n");
}

void wrapper_print_date(void)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    printf("Current date: %s", asctime(lt));
}

void wrapper_help(void)
{
    wrapper_clear();
    printf(
        "Commands:\n"
        "  where        - Show current directory\n"
        "  list         - List files and directories\n"
        "  echo         - Print text to the console\n"
        "  date         - Show current date and time\n"
        "  makedir      - Create directory\n"
        "  changedir    - Change directory\n"
        "  deletedir    - Delete directory\n"
        "  makefile     - Create file\n"
        "  deletefile   - Delete file\n"
        "  writefile    - Write text into file\n"
        "  readfile     - Read text from file\n"
        "  run          - Load a user program\n"
        "  osname       - Show OS name\n"
        "  version      - Show kernel version\n"
        "  help         - Show this help message\n");
}

void wrapper_os_name(void)
{
    struct utsname info;
    if (uname(&info) == 0)
        printf("OS name: %s\n", info.sysname);
}

void wrapper_kernel_version(void)
{
    struct utsname info;
    if (uname(&info) == 0)
        printf("Kernel version: %s\n", info.version);
}

void wrapper_shutdown(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_POWER_OFF)
        : "eax", "memory");
}

static void run(const char *name, const char **argv)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[args], %%ebx\n\t"
        "movl %[prog], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_LOAD_USER_PROGRAM),
          [prog] "r"(name),
          [args] "r"(argv)
        : "eax", "ebx", "ecx", "memory");
}

static char *copy_into_argbuf(char *argbuf, size_t bufsize, size_t *used, const char *s)
{
    if (!s)
        return NULL;

    size_t slen = strlen(s);

    if (*used + slen + 1 > bufsize)
        return NULL;

    char *dst = argbuf + *used;

    memcpy(dst, s, slen);
    dst[slen] = '\0';

    *used += slen + 1;
    return dst;
}

void wrapper_run(const char *name, const char *args)
{
    static const char *argv[64];
    static char argbuf[2048];
    int argc = 0;
    size_t used = 0;

    argbuf[0] = '\0';

    char *pname = copy_into_argbuf(argbuf, sizeof(argbuf), &used, name ? name : "");
    if (!pname)
    {
        printf("Error: program name too long.\n");
        return;
    }
    argv[argc++] = pname;

    if (args && args[0] != '\0')
    {
        char *pargs = copy_into_argbuf(argbuf, sizeof(argbuf), &used, args);
        if (!pargs)
        {
            printf("Error: args too long.\n");
            return;
        }

        char *tok = strtok(pargs, " ");
        while (tok != NULL && argc < (int)(sizeof(argv) / sizeof(argv[0])) - 1)
        {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
    }

    argv[argc] = NULL;

    run(argv[0], argv);
}
