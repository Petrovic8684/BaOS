#include <errno.h>
#include "internal/fs_errors.h"



int map_fs_error(int fs_error)
{
    switch (fs_error)
    {
    case FS_OK:
        return 0;
    case FS_ERR_NO_DRV:
        errno = ENODEV;
        return -1;
    case FS_ERR_IO:
        errno = EIO;
        return -1;
    case FS_ERR_EXISTS:
        errno = EEXIST;
        return -1;
    case FS_ERR_NO_SPACE:
        errno = ENOSPC;
        return -1;
    case FS_ERR_NAME_LONG:
        errno = ENAMETOOLONG;
        return -1;
    case FS_ERR_NOT_INIT:
        errno = ENOSYS;
        return -1;
    case FS_ERR_NO_NAME:
        errno = EINVAL;
        return -1;
    case FS_ERR_NOT_EXISTS:
        errno = ENOENT;
        return -1;
    default:
        errno = EIO;
        return -1;
    }
}
