#include "baoc_internal.h"
#include <errno.h>
#include <unistd.h>

static void trim_path(char *s)
{
    if (!s)
        return;

    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
    {
        s[n - 1] = '\0';
        n--;
    }

    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\t')
        start++;

    if (start > 0)
        memmove(s, s + start, strlen(s + start) + 1);
}

static void resolve_path(const char *in, char *out, size_t out_sz)
{
    if (!in || !out || out_sz == 0)
        return;

    if (in[0] == '/')
    {
        strncpy(out, in, out_sz - 1);
        out[out_sz - 1] = '\0';
        trim_path(out);
        return;
    }

    char *cwd = getcwd(NULL, 0);
    if (!cwd)
    {
        strncpy(out, in, out_sz - 1);
        out[out_sz - 1] = '\0';
        trim_path(out);
        return;
    }

    if (cwd[0] == '/' && cwd[1] == '\0')
        snprintf(out, out_sz, "/%s", in);
    else
        snprintf(out, out_sz, "%s/%s", cwd, in);

    free(cwd);
    out[out_sz - 1] = '\0';
    trim_path(out);
}

static int compile_one_source(const char *input_path)
{
    size_t src_sz = 0;
    if (file_read_all(input_path, src_buf, MAX_FILE_SIZE, &src_sz) != 0)
    {
        fprintf(stderr, "\033[31mError: cannot read '%s' (%s).\033[0m\n", input_path, strerror(errno));
        return -1;
    }
    src_buf[src_sz] = '\0';

    baoc_file_reset();
    preprocess_src(src_buf, preproc_buf, MAX_FILE_SIZE * 3);
    compile((const char *)preproc_buf);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "\033[31mError: No input or output file specified.\033[0m\n");
        fprintf(stderr, "Usage: baoc [-o output] file1.c [file2.c ...] output\n");
        fprintf(stderr, "       baoc input.c output\n");
        return 1;
    }

    char *out_arg = NULL;
    char *src_files[MAX_SOURCE_FILES];
    int src_cnt = 0;
    int argi = 1;

    if (strcmp(argv[argi], "-o") == 0)
    {
        if (argc < 5)
        {
            fprintf(stderr, "\033[31mError: -o requires output and at least one input.\033[0m\n");
            return 1;
        }
        out_arg = argv[argi + 1];
        argi += 2;
    }

    for (; argi < argc; argi++)
    {
        if (src_cnt >= MAX_SOURCE_FILES)
        {
            fprintf(stderr, "\033[31mError: too many source files.\033[0m\n");
            return 1;
        }
        src_files[src_cnt++] = argv[argi];
    }

    if (!out_arg)
    {
        if (src_cnt < 2)
        {
            fprintf(stderr, "\033[31mError: need input.c and output.\033[0m\n");
            return 1;
        }
        out_arg = src_files[src_cnt - 1];
        src_cnt--;
    }

    if (src_cnt < 1)
    {
        fprintf(stderr, "\033[31mError: no input files.\033[0m\n");
        return 1;
    }

    trim_path(out_arg);
    for (int i = 0; i < src_cnt; i++)
        trim_path(src_files[i]);

    for (int i = 0; i < src_cnt; i++)
    {
        if (strcmp(src_files[i], out_arg) == 0)
        {
            fprintf(stderr, "\033[31mError: Output file cannot be the same as input file.\033[0m\n");
            return 1;
        }
    }

    char output_path[512];
    resolve_path(out_arg, output_path, sizeof(output_path));

    if (baoc_memory_init() != 0)
    {
        fprintf(stderr, "\033[31mError: out of memory.\033[0m\n");
        return 1;
    }

    const char *crt0_path = "/lib/crt0";
    crt_sz = 0;
    if (file_read_all(crt0_path, crt_buf, MAX_FILE_SIZE, &crt_sz) != 0)
    {
        fprintf(stderr, "\033[31mError: cannot read crt0 '%s' (%s).\033[0m\n", crt0_path, strerror(errno));
        baoc_memory_free();
        return 1;
    }
    if (crt_sz == 0 || crt_sz > MAX_FILE_SIZE)
    {
        fprintf(stderr, "\033[31mError: crt0 size invalid.\033[0m\n");
        baoc_memory_free();
        return 1;
    }

    const char *libc_path = "/lib/libc";
    libc_sz = 0;
    if (file_read_all(libc_path, libc_buf, MAX_FILE_SIZE, &libc_sz) != 0)
    {
        fprintf(stderr, "\033[31mError: cannot read libc '%s' (%s).\033[0m\n", libc_path, strerror(errno));
        baoc_memory_free();
        return 1;
    }

    static const char *syms_paths[] = {"/lib/baoc_syms", "/lib/baoc_syms.dat", NULL};
    syms_sz = 0;
    int syms_ok = 0;
    for (int sp = 0; syms_paths[sp] && !syms_ok; sp++)
    {
        if (file_read_all(syms_paths[sp], syms_buf, MAX_FILE_SIZE, &syms_sz) == 0 &&
            baoc_libc_syms_load(syms_buf, syms_sz) == 0)
            syms_ok = 1;
    }
    if (!syms_ok)
        fprintf(stderr, "\033[1;33mWarning: cannot read /lib/baoc_syms; rebuild image (make). libc calls may fail.\033[0m\n");

    baoc_compile_reset();

    char input_path[512];
    for (int i = 0; i < src_cnt; i++)
    {
        resolve_path(src_files[i], input_path, sizeof(input_path));
        if (compile_one_source(input_path) != 0)
        {
            baoc_memory_free();
            return 1;
        }
    }

    {
        size_t log_cap = strlen((const char *)preproc_buf) + code_len * 3u + 256u;
        char *log_buf = (char *)malloc(log_cap);
        if (log_buf)
        {
            size_t n = 0;
            n += (size_t)snprintf(log_buf + n, log_cap - n,
                                  "=== Preprocessed Source ===\n%s\n=== End ===\n",
                                  (const char *)preproc_buf);
            file_write_all("baoc.log", log_buf, n);
            free(log_buf);
        }
    }

    int elf_size = baoc_link_elf(output_path);
    baoc_memory_free();
    if (elf_size < 0)
        return 1;

    printf("\033[32mWrote %s (%u bytes) ELF with crt0+libc.\033[0m\n",
           output_path, (unsigned)elf_size);
    return 0;
}
