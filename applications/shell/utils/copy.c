#include "./common/fs_common.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("\033[31mError: Copy requires source and destination.\033[0m\n");
        return 1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    char src_norm[512];
    char dst_norm[512];
    char cwd_norm[512];

    if (normalize_path(src, src_norm, sizeof(src_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize source path.\033[0m\n");
        return 1;
    }
    if (normalize_path(dst, dst_norm, sizeof(dst_norm)) != 0)
    {
        printf("\033[31mError: Failed to normalize destination path.\033[0m\n");
        return 1;
    }
    if (get_normalized_cwd(cwd_norm, sizeof(cwd_norm)) != 0)
    {
        printf("\033[31mError: Failed to get working directory.\033[0m\n");
        return 1;
    }

    if (strcmp(src_norm, "/") == 0 || strcmp(src_norm, cwd_norm) == 0)
    {
        printf("\033[31mError: Refuse to copy root or current working directory.\033[0m\n");
        return 1;
    }

    if (strcmp(src_norm, dst_norm) == 0)
    {
        printf("\033[31mError: Source and destination are the same.\033[0m\n");
        return 1;
    }

    if (is_subpath(src_norm, dst_norm))
    {
        printf("\033[31mError: Cannot copy a directory into its own subdirectory.\033[0m\n");
        return 1;
    }

    char dst_buf[512];
    const char *dst_use_norm = dst_norm;

    DIR *dstd = opendir(dst_norm);
    if (dstd)
    {
        closedir(dstd);
        const char *base = path_basename(src_norm);
        if (join_paths(dst_buf, sizeof(dst_buf), dst_norm, base) != 0)
        {
            printf("\033[31mError: Destination path too long.\033[0m\n");
            return 1;
        }
        if (normalize_path(dst_buf, dst_buf, sizeof(dst_buf)) != 0)
        {
            printf("\033[31mError: Failed to normalize constructed destination.\033[0m\n");
            return 1;
        }
        dst_use_norm = dst_buf;
    }

    if (is_subpath(src_norm, dst_use_norm))
    {
        printf("\033[31mError: Cannot copy a directory into its own subdirectory.\033[0m\n");
        return 1;
    }

    DIR *sd = opendir(src_norm);
    if (sd)
    {
        closedir(sd);
        int r = copy_dir_recursive(src_norm, dst_use_norm);
        if (r == 0)
            printf("\033[32mDirectory copied successfully.\033[0m\n");
        else
            printf("\033[31mError: Failed to copy directory.\033[0m\n");
        return 0;
    }

    int r = copy_file_internal(src_norm, dst_use_norm);
    if (r == 0)
        printf("\033[32mFile copied successfully.\033[0m\n");
    else
        printf("\033[31mError: Failed to copy file '%s' -> '%s'.\033[0m\n", src_norm, dst_use_norm);

    return 0;
}
