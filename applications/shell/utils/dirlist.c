#include <stdio.h>
#include <dirent.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *dir_path = (argc > 1 && argv[1][0] != '\0') ? argv[1] : ".";

    DIR *d = opendir(dir_path);
    if (!d)
    {
        printf("\033[31mError: Could not open directory '%s' for listing.\033[0m\n", dir_path);
        return 1;
    }

    struct dirent *ent;
    int printed_any = 0;
    while ((ent = readdir(d)) != NULL)
    {
        if (ent->d_type == DT_DIR)
            printf("\033[1;34m%s\033[0m ", ent->d_name);
        else
            printf("%s ", ent->d_name);

        printed_any = 1;
    }

    if (!printed_any)
        printf("\033[1;33m(empty directory)\033[0m");

    printf("\n");
    closedir(d);

    return 0;
}
