#include "fs_common.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys_stat.h>

int normalize_path(const char *input, char *out, size_t out_sz)
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

int join_paths(char *out, size_t out_sz, const char *base, const char *name)
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

int mkdirs_recursive(const char *path)
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

int copy_file_internal(const char *src, const char *dst)
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

int copy_dir_recursive(const char *src, const char *dst)
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

int remove_dir_recursive(const char *path)
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

bool is_subpath(const char *parent, const char *child)
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

void path_parent(const char *path, char *out, size_t out_sz)
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

const char *path_basename(const char *path)
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

int get_normalized_cwd(char *out, size_t out_sz)
{
    return normalize_path(".", out, out_sz);
}

char *where(void)
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

const char *get_current_dir_name(void)
{
    char *path = where();
    if (!path || path[0] == '\0')
        return "/";

    return path_basename(path);
}
