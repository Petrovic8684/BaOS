#include "dirlist_common.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define SYS_DIRLIST_CTX_SET 36
#define SYS_DIRLIST_CTX_GET 37

static int sys_dirlist_ctx_set(const char *path)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_DIRLIST_CTX_SET), [arg] "r"(path)
        : "eax", "ebx", "memory");
    return (int)ret;
}

void dirlist_clear_context(void)
{
    sys_dirlist_ctx_set(((const char *)0));
}

static int sys_dirlist_ctx_get(char *buf, unsigned int size)
{
    struct
    {
        char *buf;
        unsigned int size;
    } args = {buf, size};

    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_DIRLIST_CTX_GET), [arg] "r"(&args)
        : "eax", "ebx", "memory");
    return (int)ret;
}

int dirlist_resolve_path(const char *path, char *out, size_t outsz)
{
    if (!path || !out || outsz == 0)
        return -1;

    if (path[0] == '/')
    {
        strncpy(out, path, outsz - 1);
        out[outsz - 1] = '\0';
        return 0;
    }

    if (path[0] == '.' && path[1] == '\0')
    {
        char *cwd = getcwd(NULL, 0);
        if (!cwd)
            return -1;
        strncpy(out, cwd, outsz - 1);
        out[outsz - 1] = '\0';
        free(cwd);
        return 0;
    }

    char *cwd = getcwd(NULL, 0);
    if (!cwd)
        return -1;

    if (cwd[0] == '/' && cwd[1] == '\0')
        snprintf(out, outsz, "/%s", path);
    else
        snprintf(out, outsz, "%s/%s", cwd, path);

    free(cwd);
    return 0;
}

void dirlist_join_path(char *out, size_t outsz, const char *base, const char *name)
{
    if (!out || outsz == 0 || !name)
    {
        if (out && outsz > 0)
            out[0] = '\0';
        return;
    }

    if (!base || base[0] == '\0' || (base[0] == '.' && base[1] == '\0'))
    {
        snprintf(out, outsz, "%s", name);
        return;
    }

    size_t bl = strlen(base);
    if (bl > 0 && base[bl - 1] == '/')
        snprintf(out, outsz, "%s%s", base, name);
    else
        snprintf(out, outsz, "%s/%s", base, name);
}

int dirlist_save_context(const char *path)
{
    if (!path)
        return -1;

    char resolved[256];
    if (dirlist_resolve_path(path, resolved, sizeof(resolved)) != 0)
        return -1;

    DIR *d = opendir(resolved);
    if (!d)
        return -1;
    closedir(d);

    sys_dirlist_ctx_set(resolved);
    return 0;
}

int dirlist_load_context(char *buf, size_t size)
{
    if (!buf || size == 0)
        return -1;

    buf[0] = '\0';

    if (!sys_dirlist_ctx_get(buf, (unsigned int)size))
        return -1;

    return (buf[0] != '\0') ? 0 : -1;
}

int dirlist_print_contents(const char *path, FILE *out)
{
    if (!out)
        return -1;

    DIR *d = opendir(path);
    if (!d)
        return -1;

    struct dirent *ent;
    int printed_any = 0;
    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_type == DT_DIR)
            fprintf(out, "\033[1;34m%s\033[0m ", ent->d_name);
        else
            fprintf(out, "%s ", ent->d_name);

        printed_any = 1;
    }

    if (!printed_any)
        fprintf(out, "\033[1;33m(empty directory)\033[0m");

    fprintf(out, "\n");
    closedir(d);
    return 0;
}

int dirlist_collect_entries(const char *path, dir_entry_t *out, int max, int start_row)
{
    if (!out || max <= 0)
        return -1;

    DIR *d = opendir(path);
    if (!d)
        return -1;

    struct dirent *ent;
    int count = 0;
    int row = start_row;

    while ((ent = readdir(d)) != NULL && count < max)
    {
        dir_entry_t *e = &out[count];
        strncpy(e->name, ent->d_name, DIRENT_NAME_MAX);
        e->name[DIRENT_NAME_MAX] = '\0';
        e->is_dir = (ent->d_type == DT_DIR) ? 1 : 0;
        e->row = row;
        e->col = 0;
        e->len = (int)strlen(e->name);
        count++;
        row++;
    }

    closedir(d);
    return count;
}

static int is_dir_cell_attr(unsigned char attr)
{
    return (attr & 0x07) == 0x01;
}

static int is_file_cell_attr(unsigned char attr)
{
    unsigned char fg = attr & 0x0F;
    return fg == 0x07 || fg == 0x0F;
}

static int name_matches_at(int row, int col, const char *name, int is_dir)
{
    int len = (int)strlen(name);
    if (len <= 0 || col + len > 80)
        return 0;

    if (col > 0)
    {
        char before = ' ';
        unsigned char before_attr = 0x07;
        vga_get_cell(row, col - 1, &before, &before_attr);
        if (before != ' ')
            return 0;
    }

    for (int i = 0; i < len; i++)
    {
        char ch = ' ';
        unsigned char attr = 0x07;
        vga_get_cell(row, col + i, &ch, &attr);
        if (ch != name[i])
            return 0;
        if (is_dir && !is_dir_cell_attr(attr))
            return 0;
        if (!is_dir && !is_file_cell_attr(attr))
            return 0;
    }

    if (col + len < 80)
    {
        char after = ' ';
        unsigned char after_attr = 0x07;
        vga_get_cell(row, col + len, &after, &after_attr);
        if (after != ' ')
            return 0;
    }

    return 1;
}

int dirlist_scan_listing_entries(const char *base_path, dir_entry_t *out, int max)
{
    if (!base_path || !out || max <= 0)
        return -1;

    DIR *d = opendir(base_path);
    if (!d)
        return -1;

    char names[DIRENT_MAX_ENTRIES][DIRENT_NAME_MAX + 1];
    int is_dir[DIRENT_MAX_ENTRIES];
    int name_count = 0;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && name_count < DIRENT_MAX_ENTRIES)
    {
        strncpy(names[name_count], ent->d_name, DIRENT_NAME_MAX);
        names[name_count][DIRENT_NAME_MAX] = '\0';
        is_dir[name_count] = (ent->d_type == DT_DIR) ? 1 : 0;
        name_count++;
    }
    closedir(d);

    if (name_count == 0)
        return 0;

    int count = 0;

    for (int r = 0; r < 25 && count < max; r++)
    {
        for (int c = 0; c < 80 && count < max; c++)
        {
            for (int n = 0; n < name_count && count < max; n++)
            {
                if (!name_matches_at(r, c, names[n], is_dir[n]))
                    continue;

                dir_entry_t *e = &out[count];
                strncpy(e->name, names[n], DIRENT_NAME_MAX);
                e->name[DIRENT_NAME_MAX] = '\0';
                e->is_dir = is_dir[n];
                e->row = r;
                e->col = c;
                e->len = (int)strlen(e->name);
                count++;
                break;
            }
        }
    }

    return count;
}
