#include "./common/fs_common.h"
#include <stdio.h>

int main()
{
    const char *path = where();
    printf("%s\n", path);
    return 0;
}
