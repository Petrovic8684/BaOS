#include "wrappers.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys_stat.h>
#include <sys_utsname.h>

void wrapper_clear(void)
{
    printf("\033[2J");
}

void wrapper_list_dir(const char *path)
{
    const char *dir_path = (path && path[0] != '\0') ? path : ".";

    DIR *d = opendir(dir_path);
    if (!d)
    {
        printf("\033[31mError: Could not open directory '%s' for listing.\033[0m\n", dir_path);
        return;
    }

    struct dirent *ent;
    int printed_any = 0;
    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_type == DT_DIR)
            printf("\033[1;34m%s\033[0m ", ent->d_name);
        else
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
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return;
    }

    if (mkdir(name, 0755) == 0)
    {
        printf("\033[32mDirectory created successfully.\033[0m\n");
    }
    else
    {
        printf("\033[31mError: Could not create directory '%s'.\033[0m\n", name);
    }
}

static void path_parent(const char *path, char *out, size_t out_sz)
{
    if (!path || out_sz == 0)
    {
        if (out_sz)
            out[0] = '\0';
        return;
    }
    size_t n = strlen(path);
    if (n == 0)
    {
        out[0] = '\0';
        return;
    }
    while (n > 1 && path[n - 1] == '/')
        n--;
    size_t i = n;
    while (i > 0 && path[i - 1] != '/')
        i--;
    if (i == 0)
    {
        out[0] = '\0';
        return;
    }
    if (i == 1)
    {
        if (out_sz >= 2)
        {
            out[0] = '/';
            out[1] = '\0';
        }
        else
            out[0] = '\0';
        return;
    }
    size_t len = i - 1;
    if (len >= out_sz)
        len = out_sz - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

void wrapper_make_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return;
    }

    char parent[512];
    path_parent(name, parent, sizeof(parent));
    if (parent[0] != '\0')
    {
        DIR *d = opendir(parent);
        if (!d)
        {
            printf("\033[31mError: Parent directory '%s' does not exist.\033[0m\n", parent);
            return;
        }
        closedir(d);
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("\033[31mError: Could not create file '%s'.\033[0m\n", name);
        return;
    }

    if (fclose(f) != 0)
    {
        printf("\033[31mError: Failed to finalize file '%s'.\033[0m\n", name);
        return;
    }

    printf("\033[32mFile created successfully.\033[0m\n");
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

static const char *path_basename(const char *path)
{
    if (!path || path[0] == '\0')
        return "";
    const char *p = path + strlen(path) - 1;
    while (p > path && *p == '/')
        p--;
    while (p > path && *(p - 1) != '/')
        p--;
    return p;
}

const char *wrapper_get_current_dir_name(void)
{
    const char *path = where();
    if (!path || path[0] == '\0')
        return "/";

    return path_basename(path);
}

void wrapper_change_dir(const char *name)
{
    if (chdir(name) == 0)
        printf("\033[32mChanged directory successfully.\033[0m\n");
    else
        printf("\033[31mError: could not change directory to '%s'.\033[0m\n", name);
}

void wrapper_delete_dir(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return;
    }

    if (rmdir(name) == 0)
    {
        printf("\033[32mDirectory deleted successfully.\033[0m\n");
    }
    else
    {
        printf("\033[31mError: Could not delete directory '%s'.\033[0m\n", name);
    }
}

void wrapper_delete_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return;
    }

    int r = remove(name);

    if (r == 0)
        printf("\033[32mFile deleted successfully.\033[0m\n");
    else
        printf("\033[31mError: Could not delete file '%s'.\033[0m\n", name);
}

void wrapper_write_file(const char *name, const char *text)
{
    if (!name || name[0] == '\0' || !text || text[0] == '\0')
    {
        printf("\033[31mError: Invalid name or text provided.\033[0m\n");
        return;
    }

    FILE *f = fopen(name, "w");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s' for writing.\033[0m\n", name);
        return;
    }

    size_t len = 0;
    while (text[len])
        len++;

    size_t written = fwrite(text, 1, len, f);
    if (written != len)
    {
        printf("\033[31mError: I/O error occurred while writing to file '%s'.\033[0m\n", name);
        fclose(f);
        return;
    }

    const char newline = '\n';
    if (fwrite(&newline, 1, 1, f) != 1)
    {
        printf("\033[31mError: Failed to write newline to file '%s'.\033[0m\n", name);
        fclose(f);
        return;
    }

    if (fflush(f) == EOF)
    {
        printf("\033[31mError: I/O error occurred while writing to file '%s'.\033[0m\n", name);
        fclose(f);
        return;
    }

    if (fclose(f) != 0)
    {
        printf("\033[31mError: Failed to close file '%s' after writing.\033[0m\n", name);
        return;
    }

    printf("\033[32mFile written successfully.\033[0m\n");
}

void wrapper_read_file(const char *name)
{
    if (!name || name[0] == '\0')
    {
        printf("\033[31mError: Invalid name provided.\033[0m\n");
        return;
    }

    FILE *f = fopen(name, "r");
    if (!f)
    {
        printf("\033[31mError: Could not open file '%s' for reading.\033[0m\n", name);
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
        printf("\n\033[31mError: I/O error occurred while reading file '%s'.\033[0m\n", name);
        fclose(f);
        return;
    }

    if (!any)
        printf("(empty file)\n");

    fclose(f);
}

void wrapper_echo(const char *arg1, const char *arg2)
{
    if ((!arg1 || arg1[0] == 0) && (!arg2 || arg2[0] == 0))
    {
        printf("\033[31mError: Invalid text provided.\033[0m\n");
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
        "\nCommands:\n"
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
        "  copy         - Copy file or directory\n"
        "  move         - Move file or directory\n"
        "  rename       - Rename file or directory\n"
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
        printf("\033[31mError: program name too long.\033[0m\n");
        return;
    }
    argv[argc++] = pname;

    if (args && args[0] != '\0')
    {
        char *pargs = copy_into_argbuf(argbuf, sizeof(argbuf), &used, args);
        if (!pargs)
        {
            printf("\033[31mError: args too long.\033[0m\n");
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

static int mkdirs_recursive(const char *path)
{
    if (!path || path[0] == '\0')
        return 0;
    DIR *d = opendir(path);
    if (d)
    {
        closedir(d);
        return 0;
    }

    char parent[512];
    path_parent(path, parent, sizeof(parent));
    if (parent[0] != '\0')
    {
        if (mkdirs_recursive(parent) != 0)
            return -1;
    }
    int r = mkdir(path, 0755);
    return (r == 0) ? 0 : -1;
}

static int copy_file_internal(const char *src, const char *dst)
{
    if (!src || !dst)
        return -1;

    DIR *dst_dir_check = opendir(dst);
    if (dst_dir_check)
    {
        closedir(dst_dir_check);
        return -1;
    }

    FILE *fsrc = fopen(src, "r");
    if (!fsrc)
        return -1;

    char parent[512];
    path_parent(dst, parent, sizeof(parent));
    if (parent[0] != '\0')
    {
        if (mkdirs_recursive(parent) != 0)
        {
            fclose(fsrc);
            return -1;
        }
    }

    FILE *fdst = fopen(dst, "w");
    if (!fdst)
    {
        fclose(fsrc);
        return -1;
    }

    unsigned char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0)
    {
        size_t written = fwrite(buf, 1, n, fdst);
        if (written != n)
        {
            fclose(fsrc);
            fclose(fdst);
            return -1;
        }
    }

    fclose(fsrc);
    fflush(fdst);
    fclose(fdst);
    return 0;
}

static int join_paths(char *out, size_t out_sz, const char *base, const char *name)
{
    if (!out || out_sz == 0)
        return -1;
    if (!base)
        base = "";
    if (!name)
        name = "";

    size_t lb = strlen(base);
    size_t ln = strlen(name);

    if (lb == 0)
    {
        if (ln + 1 > out_sz)
            return -1;
        memcpy(out, name, ln + 1);
        return 0;
    }

    size_t lb_eff = lb;
    while (lb_eff > 1 && base[lb_eff - 1] == '/')
        lb_eff--;

    if (lb_eff == 1 && base[0] == '/')
    {
        if (1 + ln + 1 > out_sz)
            return -1;
        out[0] = '/';
        memcpy(out + 1, name, ln + 1);
        return 0;
    }

    size_t need = lb_eff + 1 + ln + 1;
    if (need > out_sz)
        return -1;

    memcpy(out, base, lb_eff);
    out[lb_eff] = '/';
    memcpy(out + lb_eff + 1, name, ln + 1);
    return 0;
}

static int copy_dir_recursive(const char *src, const char *dst)
{
    if (!src || !dst)
        return -1;

    if (mkdirs_recursive(dst) != 0)
        return -1;

    DIR *d = opendir(src);
    if (!d)
        return -1;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        char src_child[512];
        char dst_child[512];

        if (join_paths(src_child, sizeof(src_child), src, name) != 0)
        {
            closedir(d);
            return -1;
        }
        if (join_paths(dst_child, sizeof(dst_child), dst, name) != 0)
        {
            closedir(d);
            return -1;
        }

        if (ent->d_type == DT_DIR)
        {
            if (copy_dir_recursive(src_child, dst_child) != 0)
            {
                closedir(d);
                return -1;
            }
        }
        else
        {
            if (copy_file_internal(src_child, dst_child) != 0)
            {
                closedir(d);
                return -1;
            }
        }
    }

    closedir(d);
    return 0;
}

static int remove_dir_recursive(const char *path)
{
    if (!path)
        return -1;

    DIR *d = opendir(path);
    if (!d)
    {
        if (remove(path) == 0)
            return 0;
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL)
    {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        char child[512];
        if (join_paths(child, sizeof(child), path, name) != 0)
        {
            closedir(d);
            return -1;
        }

        if (ent->d_type == DT_DIR)
        {
            if (remove_dir_recursive(child) != 0)
            {
                closedir(d);
                return -1;
            }
            if (rmdir(child) != 0)
            {
                closedir(d);
                return -1;
            }
        }
        else
        {
            if (remove(child) != 0)
            {
                closedir(d);
                return -1;
            }
        }
    }

    closedir(d);
    if (rmdir(path) == 0)
        return 0;
    return -1;
}

static int normalize_path(const char *input, char *out, size_t out_sz)
{
    if (!input || !out || out_sz == 0)
        return -1;

    char temp[512];
    char cwd[512];

    if (input[0] == '/')
    {
        size_t li = strlen(input);
        if (li + 1 > sizeof(temp))
            return -1;
        memcpy(temp, input, li + 1);
    }
    else
    {
        if (!getcwd(cwd, sizeof(cwd)))
            return -1;
        size_t lc = strlen(cwd);
        size_t li = strlen(input);
        if (lc == 1 && cwd[0] == '/')
        {
            if (1 + li + 1 > sizeof(temp))
                return -1;
            temp[0] = '/';
            memcpy(temp + 1, input, li + 1);
        }
        else
        {
            if (lc + 1 + li + 1 > sizeof(temp))
                return -1;
            memcpy(temp, cwd, lc);
            temp[lc] = '/';
            memcpy(temp + lc + 1, input, li + 1);
        }
    }

    out[0] = '\0';
    size_t tlen = strlen(temp);
    size_t i = 0;
    while (i < tlen)
    {
        while (i < tlen && temp[i] == '/')
            i++;
        if (i >= tlen)
            break;

        size_t start = i;
        while (i < tlen && temp[i] != '/')
            i++;
        size_t comp_len = i - start;
        if (comp_len == 0)
            continue;

        if (comp_len == 1 && temp[start] == '.')
            continue;

        else if (comp_len == 2 && temp[start] == '.' && temp[start + 1] == '.')
        {
            size_t olen = strlen(out);
            if (olen == 0 || (olen == 1 && out[0] == '/'))
            {
                out[0] = '/';
                out[1] = '\0';
            }
            else
            {
                int last_slash = -1;
                for (int j = (int)olen - 1; j >= 0; --j)
                {
                    if (out[j] == '/')
                    {
                        last_slash = j;
                        break;
                    }
                }
                if (last_slash <= 0)
                {
                    out[0] = '/';
                    out[1] = '\0';
                }
                else
                {
                    out[last_slash] = '\0';
                }
            }
            continue;
        }
        else
        {
            size_t olen = strlen(out);
            if (olen == 0)
            {
                if (1 + comp_len + 1 > out_sz)
                    return -1;
                out[0] = '/';
                memcpy(out + 1, temp + start, comp_len);
                out[1 + comp_len] = '\0';
            }
            else if (olen == 1 && out[0] == '/')
            {
                if (1 + comp_len + 1 > out_sz)
                    return -1;
                memcpy(out + 1, temp + start, comp_len);
                out[1 + comp_len] = '\0';
            }
            else
            {
                if (olen + 1 + comp_len + 1 > out_sz)
                    return -1;
                out[olen] = '/';
                memcpy(out + olen + 1, temp + start, comp_len);
                out[olen + 1 + comp_len] = '\0';
            }
        }
    }
    if (out[0] == '\0')
    {
        if (out_sz < 2)
            return -1;
        out[0] = '/';
        out[1] = '\0';
    }

    size_t olen = strlen(out);
    if (olen > 1 && out[olen - 1] == '/')
    {
        out[olen - 1] = '\0';
    }

    return 0;
}

static bool is_subpath(const char *parent, const char *child)
{
    if (!parent || !child)
        return false;

    if (strcmp(parent, "/") == 0)
        return true;

    size_t lp = strlen(parent);
    size_t lc = strlen(child);

    if (lc < lp)
        return false;

    if (strncmp(parent, child, lp) != 0)
        return false;

    if (child[lp] == '/' || child[lp] == '\0')
        return true;

    return false;
}

static int get_normalized_cwd(char *out, size_t out_sz)
{
    return normalize_path(".", out, out_sz);
}

void wrapper_copy(const char *src, const char *dst)
{
    if (!src || !dst || src[0] == '\0' || dst[0] == '\0')
    {
        printf("\033[31mError: Copy requires source and destination.\033[0m\n");
        return;
    }

    char src_norm[512];
    char dst_norm[512];
    char cwd_norm[512];

    if (normalize_path(src, src_norm, sizeof(src_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize source path.\033[0m\n");
        return;
    }
    if (normalize_path(dst, dst_norm, sizeof(dst_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize destination path.\033[0m\n");
        return;
    }
    if (get_normalized_cwd(cwd_norm, sizeof(cwd_norm)) != 0)
    {
        printf("\033[31mError: Failed to get working directory.\033[0m\n");
        return;
    }

    if (strcmp(src_norm, "/") == 0 || strcmp(src_norm, cwd_norm) == 0)
    {
        printf("\033[31mError: Refuse to copy root or current working directory.\033[0m\n");
        return;
    }

    if (strcmp(src_norm, dst_norm) == 0)
    {
        printf("\033[31mError: Source and destination are the same.\033[0m\n");
        return;
    }

    if (is_subpath(src_norm, dst_norm))
    {
        printf("\033[31mError: Cannot copy a directory into its own subdirectory.\033[0m\n");
        return;
    }

    char dst_buf[512];
    const char *dst_use_norm = dst_norm;

    DIR *dstd = opendir(dst_norm);
    if (dstd)
    {
        closedir(dstd);
        const char *base = path_basename(src_norm);
        if (join_paths(dst_buf, sizeof(dst_buf), dst_norm, base) != 0)
        {
            printf("\033[31mError: Destination path too long.\033[0m\n");
            return;
        }
        if (normalize_path(dst_buf, dst_buf, sizeof(dst_buf)) != 0)
        {
            printf("\033[31mError: Failed to normalize constructed destination.\033[0m\n");
            return;
        }
        dst_use_norm = dst_buf;
    }

    if (is_subpath(src_norm, dst_use_norm))
    {
        printf("\033[31mError: Cannot copy a directory into its own subdirectory.\033[0m\n");
        return;
    }

    DIR *sd = opendir(src_norm);
    if (sd)
    {
        closedir(sd);
        int r = copy_dir_recursive(src_norm, dst_use_norm);
        if (r == 0)
            printf("\033[32mDirectory copied successfully.\033[0m\n");
        else
            printf("\033[31mError: Failed to copy directory.\033[0m\n");
        return;
    }

    int r = copy_file_internal(src_norm, dst_use_norm);
    if (r == 0)
        printf("\033[32mFile copied successfully.\033[0m\n");
    else
        printf("\033[31mError: Failed to copy file '%s' -> '%s'.\033[0m\n", src_norm, dst_use_norm);
}

void wrapper_move(const char *src, const char *dst)
{
    if (!src || !dst || src[0] == '\0' || dst[0] == '\0')
    {
        printf("\033[31mError: Move requires source and destination.\033[0m\n");
        return;
    }

    char src_norm[512];
    char dst_norm[512];
    char cwd_norm[512];

    if (normalize_path(src, src_norm, sizeof(src_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize source path.\033[0m\n");
        return;
    }
    if (normalize_path(dst, dst_norm, sizeof(dst_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize destination path.\033[0m\n");
        return;
    }
    if (get_normalized_cwd(cwd_norm, sizeof(cwd_norm)) != 0)
    {
        printf("\033[31mError: Failed to get working directory.\033[0m\n");
        return;
    }

    if (strcmp(src_norm, "/") == 0 || strcmp(src_norm, cwd_norm) == 0)
    {
        printf("\033[31mError: Refuse to move root or current working directory.\033[0m\n");
        return;
    }

    if (strcmp(src_norm, dst_norm) == 0)
    {
        printf("\033[31mError: Source and destination are the same.\033[0m\n");
        return;
    }

    if (is_subpath(src_norm, dst_norm))
    {
        printf("\033[31mError: Cannot move a directory into its own subdirectory.\033[0m\n");
        return;
    }

    char dst_buf[512];
    const char *dst_use_norm = dst_norm;

    DIR *dstd = opendir(dst_norm);
    if (dstd)
    {
        closedir(dstd);
        const char *base = path_basename(src_norm);
        if (join_paths(dst_buf, sizeof(dst_buf), dst_norm, base) != 0)
        {
            printf("\033[31mError: Destination path too long.\033[0m\n");
            return;
        }
        if (normalize_path(dst_buf, dst_buf, sizeof(dst_buf)) != 0)
        {
            printf("\033[31mError: Failed to normalize constructed destination.\033[0m\n");
            return;
        }
        dst_use_norm = dst_buf;
    }

    if (is_subpath(src_norm, dst_use_norm))
    {
        printf("\033[31mError: Cannot move a directory into its own subdirectory.\033[0m\n");
        return;
    }

    if (rename(src_norm, dst_use_norm) == 0)
    {
        printf("\033[32mMoved successfully.\033[0m\n");
        return;
    }

    DIR *sd = opendir(src_norm);
    if (sd)
    {
        closedir(sd);
        if (copy_dir_recursive(src_norm, dst_use_norm) == 0)
        {
            if (remove_dir_recursive(src_norm) == 0)
            {
                printf("\033[32mDirectory moved successfully.\033[0m\n");
                return;
            }
            else
            {
                printf("\033[31mError: Copied but failed to remove source directory.\033[0m\n");
                return;
            }
        }
        else
        {
            printf("\033[31mError: Failed to copy directory for move.\033[0m\n");
            return;
        }
    }
    else
    {
        if (copy_file_internal(src_norm, dst_use_norm) == 0)
        {
            if (remove(src_norm) == 0)
            {
                printf("\033[32mFile moved successfully.\033[0m\n");
                return;
            }
            else
            {
                printf("\033[31mError: Copied but failed to delete source file.\033[0m\n");
                return;
            }
        }
        else
        {
            printf("\033[31mError: Failed to copy file for move.\033[0m\n");
            return;
        }
    }
}