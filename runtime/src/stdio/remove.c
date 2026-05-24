#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "internal/fs_helpers.h"

int remove(const char *pathname)
{
    if (!pathname)
    {
        errno = EINVAL;
        return -1;
    }
    int res = fs_delete_file(pathname);
    return (res == 0) ? 0 : -1;
}
