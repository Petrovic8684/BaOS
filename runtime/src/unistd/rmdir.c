#include <unistd.h>
#include "internal/fs_helpers.h"

int rmdir(const char *path)
{
    int ret = fs_delete_dir(path);

    if (ret == 0)
        return 0;

    return -1;
}
