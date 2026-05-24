#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "internal/fs_helpers.h"

char *realpath(const char *path, char *resolved_path)
{
    if (!path)
    {
        errno = EINVAL;
        return NULL;
    }

    char *cwd = NULL;
    if (path[0] != '/')
    {
        cwd = fs_where();
        if (!cwd)
            return NULL;
    }

    size_t abs_len = strlen(path) + (cwd ? strlen(cwd) + 1 : 0) + 1;
    char *abs_path = malloc(abs_len);
    if (!abs_path)
    {
        free(cwd);
        return NULL;
    }

    if (path[0] == '/')
        strcpy(abs_path, path);
    else
    {
        strcpy(abs_path, cwd);
        if (cwd[strlen(cwd) - 1] != '/')
            strcat(abs_path, "/");
        strcat(abs_path, path);
    }

    free(cwd);

    char **stack = NULL;
    size_t stack_size = 0, stack_cap = 0;

    char *copy = strdup(abs_path);
    free(abs_path);
    if (!copy)
        return NULL;

    char *token = strtok(copy, "/");
    while (token)
    {
        if (strcmp(token, ".") == 0)
        {
            token = strtok(NULL, "/");
            continue;
        }
        if (strcmp(token, "..") == 0)
        {
            if (stack_size > 0)
                stack_size--;
            token = strtok(NULL, "/");
            continue;
        }

        if (stack_size == stack_cap)
        {
            size_t new_cap = stack_cap ? stack_cap * 2 : 8;
            char **tmp = realloc(stack, new_cap * sizeof(char *));
            if (!tmp)
            {
                free(stack);
                free(copy);
                return NULL;
            }
            stack = tmp;
            stack_cap = new_cap;
        }
        stack[stack_size++] = token;
        token = strtok(NULL, "/");
    }

    size_t total_len = 1;
    for (size_t i = 0; i < stack_size; i++)
        total_len += strlen(stack[i]) + 1;

    char *result = resolved_path;
    if (!resolved_path)
        result = malloc(total_len);
    if (!result)
    {
        free(stack);
        free(copy);
        return NULL;
    }

    result[0] = '/';
    result[1] = '\0';

    for (size_t i = 0; i < stack_size; i++)
    {
        strcat(result, stack[i]);
        if (i != stack_size - 1)
            strcat(result, "/");
    }

    free(stack);
    free(copy);
    return result;
}
