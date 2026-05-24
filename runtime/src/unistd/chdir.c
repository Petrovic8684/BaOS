#include <unistd.h>
#include "internal/fs_helpers.h"

int chdir(const char *path)
{
    return fs_change_dir(path);
}
