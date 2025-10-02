#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define SYS_LOAD_USER_PROGRAM 17

#define MAX_ARGC 64
#define MAX_ARGV_LEN 128

static void run(const char *name, const char *args)
{
    if (!name)
        name = "";

    int argc = 1;
    if (args && args[0] != '\0')
    {
        char *tmp = strdup(args);
        if (!tmp)
        {
            printf("\033[31mError: Memory allocation failed. %s.\033[0m\n\n", strerror(errno));
            return;
        }
        char *t = strtok(tmp, " ");
        while (t)
        {
            argc++;
            t = strtok(NULL, " ");
        }
        free(tmp);
    }

    if (argc > MAX_ARGC)
    {
        printf("\033[31mError: Too many arguments (max %d allowed).\033[0m\n\n", MAX_ARGC);
        return;
    }

    const char *prog_basename = name;
    const char *slash = strrchr(name, '/');
    if (slash)
        prog_basename = slash + 1;

    if (strlen(prog_basename) >= MAX_ARGV_LEN)
    {
        printf("\033[31mError: Program name too long (max %d chars).\033[0m\n\n", MAX_ARGV_LEN);
        return;
    }

    size_t total_size = (size_t)(argc + 1) * sizeof(char *);
    total_size += strlen(name) + 1;
    if (args && args[0] != '\0')
        total_size += strlen(args) + 1;

    char *block = malloc(total_size);
    if (!block)
    {
        printf("\033[31mError: Memory allocation failed. %s.\033[0m\n\n", strerror(errno));
        return;
    }

    char **argv = (char **)block;
    char *str_area = block + (argc + 1) * sizeof(char *);

    argv[0] = str_area;
    strcpy(argv[0], name);
    str_area += strlen(name) + 1;

    argc = 1;
    if (args && args[0] != '\0')
    {
        char *p = strdup(args);
        if (!p)
        {
            free(block);
            printf("\033[31mError: Failed to duplicate arguments. %s.\033[0m\n\n", strerror(errno));
            return;
        }

        char *tok = strtok(p, " ");
        while (tok)
        {
            if (strlen(tok) >= MAX_ARGV_LEN)
            {
                printf("\033[31mError: Argument '%s' too long (max %d chars).\033[0m\n\n", tok, MAX_ARGV_LEN);
                free(p);
                free(block);
                return;
            }

            argv[argc] = str_area;
            strcpy(argv[argc], tok);
            str_area += strlen(tok) + 1;
            argc++;

            tok = strtok(NULL, " ");
        }
        free(p);
    }

    argv[argc] = NULL;

    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[args], %%ebx\n\t"
        "movl %[prog], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_LOAD_USER_PROGRAM),
          [prog] "r"(argv[0]),
          [args] "r"(argv)
        : "eax", "ebx", "ecx", "memory");

    free(block);
}

void process_command(char *cmd)
{
    while (*cmd == ' ')
        cmd++;
    if (!*cmd)
        return;

    char *p = cmd;
    while (*p && *p != ' ')
        p++;

    char saved = *p;
    *p = '\0';
    char *command = cmd;
    char *args = (saved != '\0') ? p + 1 : NULL;

    const char *paths[] = {".", "/programs", "/programs/utils"};
    size_t paths_n = sizeof(paths) / sizeof(paths[0]);
    bool launched = false;

    for (int pidx = 0; pidx < paths_n && !launched; ++pidx)
    {
        DIR *d = opendir(paths[pidx]);
        if (!d)
            continue;

        struct dirent *ent;
        while ((ent = readdir(d)))
        {
            if (strcmp(ent->d_name, command) != 0)
                continue;

            size_t path_len = strlen(paths[pidx]) + 1 + strlen(ent->d_name) + 1;
            char *prog_path = malloc(path_len);
            if (!prog_path)
                continue;
            snprintf(prog_path, path_len, "%s/%s", paths[pidx], ent->d_name);

            closedir(d);
            launched = true;

            run(prog_path, args && *args ? args : NULL);
            free(prog_path);
            break;
        }

        if (!launched)
            closedir(d);
    }

    if (!launched)
        printf("\033[31mError: Unknown command. Type 'help' for a list of valid commands.\033[0m\n\n");
}

void main(void)
{
    while (true)
    {
        char *cwd = getcwd(NULL, 0);
        if (!cwd)
            cwd = strdup("?");

        char *base = basename(cwd);
        printf("$ %s> ", base);

        free(base);
        free(cwd);

        char *buffer = NULL;
        read_line(&buffer);
        if (!buffer)
            continue;

        printf("\n");
        process_command(buffer);

        free(buffer);
    }
}
