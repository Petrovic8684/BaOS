#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("\033[31mError: Invalid source or destination provided.\033[0m\n");
        return 1;
    }

    const char *src = argv[1];
    const char *dest = argv[2];

    char *src_abs = realpath(src, NULL);
    if (!src_abs)
    {
        printf("\033[31mError: Could not resolve source path '%s'.\033[0m\n", src);
        return 1;
    }

    struct stat st;
    char *final_dest = NULL;
    if (stat(dest, &st) == 0 && S_ISDIR(st.st_mode))
    {
        char *src_copy = strdup(src_abs);
        if (!src_copy)
        {
            printf("\033[31mError: Memory allocation failed.\033[0m\n");
            free(src_abs);
            return 1;
        }
        char *base = basename(src_copy);
        size_t len = strlen(dest) + 1 + strlen(base) + 1;
        final_dest = malloc(len);
        if (!final_dest)
        {
            printf("\033[31mError: Memory allocation failed.\033[0m\n");
            free(src_abs);
            free(src_copy);
            return 1;
        }
        snprintf(final_dest, len, "%s/%s", dest, base);
        free(src_copy);
    }
    else
    {
        final_dest = strdup(dest);
        if (!final_dest)
        {
            printf("\033[31mError: Memory allocation failed.\033[0m\n");
            free(src_abs);
            return 1;
        }
    }

    char *dest_copy = strdup(final_dest);
    if (!dest_copy)
    {
        printf("\033[31mError: Memory allocation failed.\033[0m\n");
        free(src_abs);
        free(final_dest);
        return 1;
    }
    char *dest_base = basename(dest_copy);

    if (strlen(dest_base) > 16)
    {
        printf("\033[31mError: Name exceeds maximum length of 16 characters.\033[0m\n");
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }
    for (size_t i = 0; i < strlen(dest_base); i++)
        if (!isalnum((unsigned char)dest_base[i]) && (unsigned char)dest_base[i] != '.')
        {
            printf("\033[31mError: Name must contain only letters and digits.\033[0m\n");
            free(src_abs);
            free(final_dest);
            free(dest_copy);
            return 1;
        }

    FILE *f_src = fopen(src_abs, "rb");
    if (!f_src)
    {
        printf("\033[31mError: Could not open source file '%s'.\033[0m\n", src_abs);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    struct stat st_dest;
    if (stat(final_dest, &st_dest) == 0)
    {
        printf("\033[31mError: Destination file '%s' already exists.\033[0m\n", final_dest);
        fclose(f_src);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    if (fseek(f_src, 0, SEEK_END) != 0)
    {
        printf("\033[31mError: fseek failed on source file.\033[0m\n");
        fclose(f_src);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    long sz = ftell(f_src);
    if (sz < 0)
    {
        printf("\033[31mError: ftell failed on source file.\033[0m\n");
        fclose(f_src);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    if (fseek(f_src, 0, SEEK_SET) != 0)
    {
        printf("\033[31mError: fseek failed on source file.\033[0m\n");
        fclose(f_src);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    size_t size = (size_t)sz;
    unsigned char *buffer = NULL;
    if (size > 0)
    {
        buffer = malloc(size);
        if (!buffer)
        {
            printf("\033[31mError: Memory allocation failed.\033[0m\n");
            fclose(f_src);
            free(src_abs);
            free(final_dest);
            free(dest_copy);
            return 1;
        }
        if (fread(buffer, 1, size, f_src) != size)
        {
            printf("\033[31mError: Failed to read entire source file.\033[0m\n");
            fclose(f_src);
            free(buffer);
            free(src_abs);
            free(final_dest);
            free(dest_copy);
            return 1;
        }
    }

    fclose(f_src);

    FILE *f_dest = fopen(final_dest, "wb");
    if (!f_dest)
    {
        printf("\033[31mError: Could not open destination file '%s'.\033[0m\n", final_dest);
        free(buffer);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    if (size > 0 && fwrite(buffer, 1, size, f_dest) != size)
    {
        printf("\033[31mError: Failed to write to destination file.\033[0m\n");
        fclose(f_dest);
        free(buffer);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    fclose(f_dest);

    if (remove(src_abs) != 0)
    {
        printf("\033[31mError: Failed to delete source file '%s'.\033[0m\n", src_abs);
        free(buffer);
        free(src_abs);
        free(final_dest);
        free(dest_copy);
        return 1;
    }

    printf("\033[32mSuccessfully moved '%s' (%u bytes) to '%s'.\033[0m\n", src_abs, size, final_dest);

    free(buffer);
    free(src_abs);
    free(final_dest);
    free(dest_copy);
    return 0;
}
