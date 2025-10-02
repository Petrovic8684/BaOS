#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("\033[1;33mUsage:\033[0m filedelete <file>.\n");
        return 1;
    }

    const char *file_name = argv[1];
    int r = remove(file_name);

    if (r == 0)
        printf("\033[32mFile deleted successfully.\033[0m\n");
    else
        printf("\033[31mError: Could not delete file '%s'. %s.\033[0m\n", file_name, strerror(errno));

    return 0;
}
