#include <dirent.h>
#include <stddef.h>
#include <string.h>

#define SYS_FS_WHERE 11
#define SYS_FS_CHANGE_DIR 16
#define SYS_FS_LIST_DIR 15

#define USER_BUFFER_SIZE 1024

static inline const char *fs_where(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[buffer], %%ebx\n\t"
        "movl %[size], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_WHERE),
          [buffer] "r"(buffer),
          [size] "r"(USER_BUFFER_SIZE)
        : "eax", "ebx", "ecx", "memory");
    return buffer;
}

static inline const char *fs_list_dir(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[buffer], %%ebx\n\t"
        "movl %[size], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_LIST_DIR),
          [buffer] "r"(buffer),
          [size] "r"(USER_BUFFER_SIZE)
        : "eax", "ebx", "ecx", "memory");
    return buffer;
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

        unsigned int j = 0;
        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n' && j < DIRENT_NAME_MAX)
            d->names[out][j++] = list[i++];

        d->names[out][j] = '\0';

        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n')
            i++;

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

    char original[USER_BUFFER_SIZE];
    const char *orig_p = fs_where();
    if (orig_p)
    {
        unsigned int k = 0;
        while (k < USER_BUFFER_SIZE - 1 && orig_p[k] != '\0')
        {
            original[k] = orig_p[k];
            k++;
        }
        original[k] = '\0';
    }
    else
        original[0] = '\0';

    int changed = 0;
    if (name && name[0] != '\0' && !(name[0] == '.' && name[1] == '\0'))
    {
        int ch = fs_change_dir(name);
        if (ch != 0)
        {
            d->in_use = 0;
            return NULL;
        }
        changed = 1;
    }

    const char *listing = fs_list_dir();
    parse_list_into_dir(d, listing);

    if (changed)
    {
        for (int iter = 0; iter < 128; iter++)
        {
            const char *now = fs_where();
            if (!now)
                break;
            if (strncmp(now, original, USER_BUFFER_SIZE) == 0)
                break;

            fs_change_dir("..");
        }
    }

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
