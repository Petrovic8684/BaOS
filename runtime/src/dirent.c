#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define SYS_FS_WHERE 8
#define SYS_FS_LIST_DIR 9
#define SYS_FS_CHANGE_DIR 10

static unsigned int fs_where_len(void)
{
    unsigned int len;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl $0, %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(len)
        : [num] "i"(SYS_FS_WHERE)
        : "eax", "ebx", "memory");
    return len;
}

static char *fs_where(void)
{
    unsigned int len = fs_where_len();
    if (len == 0)
        return NULL;

    char *buf = malloc(len);
    if (!buf)
        return NULL;

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[ptr], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_WHERE), [ptr] "r"(buf)
        : "eax", "ebx", "memory");

    return buf;
}

static unsigned int fs_list_dir_len(void)
{
    unsigned int len;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl $0, %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(len)
        : [num] "i"(SYS_FS_LIST_DIR)
        : "eax", "ebx", "memory");
    return len;
}

static char *fs_list_dir(void)
{
    unsigned int len = fs_list_dir_len();
    if (len == 0)
        return NULL;

    char *buf = malloc(len);
    if (!buf)
        return NULL;

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[ptr], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_LIST_DIR),
          [ptr] "r"(buf)
        : "eax", "ebx", "memory");

    return buf;
}

static inline int fs_change_dir(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_CHANGE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

struct DIR
{
    int in_use;
    char names[DIRENT_MAX_ENTRIES][DIRENT_NAME_MAX + 1];
    unsigned char types[DIRENT_MAX_ENTRIES];
    unsigned int count;
    long pos;
    struct dirent dent;
};

static struct DIR dir_streams[DIRENT_MAX_STREAMS];

static int find_free_stream(void)
{
    for (int i = 0; i < DIRENT_MAX_STREAMS; i++)
        if (!dir_streams[i].in_use)
            return i;

    return -1;
}

static unsigned int parse_list_into_dir(struct DIR *d, const char *list)
{
    d->count = 0;
    d->pos = 0;

    if (!list)
        return 0;

    if (list[0] == '(')
        return 0;

    unsigned int out = 0;
    unsigned int i = 0;
    while (list[i] != '\0' && out < DIRENT_MAX_ENTRIES)
    {
        while (list[i] == ' ' || list[i] == '\t' || list[i] == '\r' || list[i] == '\n')
            i++;
        if (list[i] == '\0')
            break;

        char temp[DIRENT_NAME_MAX + 2];
        unsigned int j = 0;
        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n' && j < (DIRENT_NAME_MAX + 1))
            temp[j++] = list[i++];
        temp[j] = '\0';

        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n')
            i++;

        if (temp[0] == '\0')
            continue;

        unsigned char typ = DT_UNKNOWN;
        unsigned int actual_len = j;
        if (actual_len > 0 && temp[actual_len - 1] == '/')
        {
            typ = DT_DIR;
            temp[actual_len - 1] = '\0';
        }
        else
            typ = DT_REG;

        unsigned int k = 0;
        for (; k < DIRENT_NAME_MAX && temp[k] != '\0'; k++)
            d->names[out][k] = temp[k];
        d->names[out][k] = '\0';

        d->types[out] = typ;

        if (d->names[out][0] != '\0')
            out++;
    }

    d->count = out;
    return out;
}

DIR *opendir(const char *name)
{
    int idx = find_free_stream();
    if (idx < 0)
        return NULL;

    struct DIR *d = &dir_streams[idx];
    d->in_use = 1;
    d->count = 0;
    d->pos = 0;
    d->dent.d_ino = 0;
    d->dent.d_name[0] = '\0';
    for (unsigned int i = 0; i < DIRENT_MAX_ENTRIES; i++)
        d->types[i] = DT_UNKNOWN;

    char *orig_p = fs_where();
    if (!orig_p)
        orig_p = strdup("/");

    int changed = 0;
    if (name && name[0] != '\0' && !(name[0] == '.' && name[1] == '\0'))
    {
        int ch = fs_change_dir(name);
        if (ch != 0)
        {
            free(orig_p);
            d->in_use = 0;
            return NULL;
        }
        changed = 1;
    }

    char *listing = fs_list_dir();
    parse_list_into_dir(d, listing);

    if (changed && orig_p)
        fs_change_dir(orig_p);

    free(orig_p);
    free(listing);

    return (DIR *)d;
}

struct dirent *readdir(DIR *dirp)
{
    if (!dirp)
        return NULL;

    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return NULL;

    if ((unsigned long)d->pos >= d->count)
        return NULL;

    d->dent.d_ino = (unsigned long)(d->pos + 1);

    unsigned int i = 0;
    for (; i < DIRENT_NAME_MAX && d->names[d->pos][i] != '\0'; i++)
        d->dent.d_name[i] = d->names[d->pos][i];
    d->dent.d_name[i] = '\0';

    d->dent.d_type = d->types[d->pos];

    d->pos++;
    return &d->dent;
}

int closedir(DIR *dirp)
{
    if (!dirp)
        return -1;

    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return -1;

    d->in_use = 0;
    d->count = 0;
    d->pos = 0;
    d->dent.d_name[0] = '\0';
    d->dent.d_ino = 0;
    for (unsigned int i = 0; i < DIRENT_MAX_ENTRIES; i++)
        d->types[i] = DT_UNKNOWN;

    return 0;
}

void rewinddir(DIR *dirp)
{
    if (!dirp)
        return;
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return;
    d->pos = 0;
}

long telldir(DIR *dirp)
{
    if (!dirp)
        return -1;
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return -1;
    return d->pos;
}

void seekdir(DIR *dirp, long loc)
{
    if (!dirp)
        return;
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return;
    if (loc < 0)
        d->pos = 0;
    else if ((unsigned long)loc > d->count)
        d->pos = d->count;
    else
        d->pos = loc;
}
