#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "dirlist_common.h"

int main(int argc, char *argv[])
{
    const char *dir_path = (argc > 1 && argv[1][0] != '\0') ? argv[1] : ".";

    if (dirlist_print_contents(dir_path, stdout) != 0)
    {
        printf("\033[31mError: Could not open directory '%s' for listing. %s.\033[0m\n", dir_path, strerror(errno));
        return 1;
    }

    dirlist_save_context(dir_path);
    return 0;
}
