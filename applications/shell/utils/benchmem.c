#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#define SYS_HEAP_INFO 20
#define MAX_FRAG_BLOCKS 512

typedef struct
{
    const char *name;
    unsigned long long elapsed_cycles;
    unsigned long elapsed_ms;
    unsigned long iterations;
    unsigned long failures;
    unsigned long payload_bytes;
    user_heap_info_t user_hi;
    user_heap_info_t kernel_hi;
} bench_result_t;

static unsigned long long rdtsc64(void)
{
    unsigned int lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
}

static unsigned long now_ms(void)
{
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return 0;
    return info.uptime;
}

static unsigned int div10_u64(unsigned long long *n)
{
    unsigned long long q = 0;
    unsigned long long r = 0;
    int i;

    for (i = 63; i >= 0; i--)
    {
        r = (r << 1) | ((*n >> i) & 1ULL);
        q <<= 1;
        if (r >= 10ULL)
        {
            r -= 10ULL;
            q |= 1ULL;
        }
    }

    *n = q;
    return (unsigned int)r;
}

static void print_ull(unsigned long long v)
{
    char buf[21];
    int i = 20;

    buf[i] = '\0';
    if (v == 0)
    {
        printf("0");
        return;
    }

    while (v > 0 && i > 0)
    {
        unsigned int digit = div10_u64(&v);
        buf[--i] = (char)('0' + digit);
    }

    printf("%s", &buf[i]);
}

static int get_kernel_heap_info(user_heap_info_t *info)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_HEAP_INFO), [arg] "r"(info)
        : "eax", "ebx", "memory");

    return ret;
}

static void collect_heap_stats(bench_result_t *r)
{
    get_user_heap_info(&r->user_hi);
    if (get_kernel_heap_info(&r->kernel_hi) != 0)
    {
        r->kernel_hi.heap_start = 0;
        r->kernel_hi.heap_end = 0;
        r->kernel_hi.heap_max = 0;
        r->kernel_hi.free_bytes = 0;
    }
}

static void print_result(const bench_result_t *r)
{
    unsigned int arena = r->user_hi.heap_end - r->user_hi.heap_start;
    unsigned int used = arena - r->user_hi.free_bytes;
    unsigned int overhead = 0;

    if (used > r->payload_bytes)
        overhead = used - (unsigned int)r->payload_bytes;

    printf("\033[1;33m=== benchmem: %s ===\033[0m\n", r->name);
    printf("elapsed_cycles: ");
    print_ull(r->elapsed_cycles);
    printf("\n");
    printf("elapsed_ms: %lu\n", r->elapsed_ms);
    printf("iterations: %lu\n", r->iterations);
    printf("failures: %lu\n", r->failures);
    printf("user_heap_start: 0x%x\n", r->user_hi.heap_start);
    printf("user_heap_end: 0x%x\n", r->user_hi.heap_end);
    printf("user_arena_bytes: %u\n", arena);
    printf("user_used_bytes: %u\n", used);
    printf("user_free_bytes: %u\n", r->user_hi.free_bytes);
    printf("kernel_heap_end: 0x%x\n", r->kernel_hi.heap_end);
    printf("payload_bytes: %lu\n", r->payload_bytes);
    printf("arena_overhead_bytes: %u\n\n", overhead);
}

static unsigned long parse_ulong(const char *s, unsigned long default_val)
{
    if (!s || !*s)
        return default_val;

    char *end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s)
        return default_val;
    return v;
}

static void bench_expand(unsigned long count, unsigned long size)
{
    bench_result_t r;
    r.name = "expand";
    r.iterations = 0;
    r.failures = 0;
    r.payload_bytes = 0;

    unsigned long t0_ms = now_ms();
    unsigned long long t0 = rdtsc64();

    for (unsigned long i = 0; i < count; i++)
    {
        void *p = malloc((unsigned int)size);
        if (!p)
        {
            r.failures++;
            break;
        }

        *(volatile char *)p = 0;
        r.iterations++;
        r.payload_bytes += size;
    }

    r.elapsed_cycles = rdtsc64() - t0;
    r.elapsed_ms = now_ms() - t0_ms;
    collect_heap_stats(&r);
    print_result(&r);
}

static void bench_churn(unsigned long count, unsigned long size)
{
    bench_result_t r;
    r.name = "churn";
    r.iterations = 0;
    r.failures = 0;
    r.payload_bytes = count * size;

    unsigned long t0_ms = now_ms();
    unsigned long long t0 = rdtsc64();

    for (unsigned long i = 0; i < count; i++)
    {
        void *p = malloc((unsigned int)size);
        if (!p)
        {
            r.failures++;
            break;
        }

        *(volatile char *)p = 0;
        free(p);
        r.iterations++;
    }

    r.elapsed_cycles = rdtsc64() - t0;
    r.elapsed_ms = now_ms() - t0_ms;
    collect_heap_stats(&r);
    print_result(&r);
}

static void bench_frag(unsigned long count, unsigned long size)
{
    bench_result_t r;
    r.name = "frag";
    r.iterations = 0;
    r.failures = 0;
    r.payload_bytes = 0;

    if (count > MAX_FRAG_BLOCKS)
        count = MAX_FRAG_BLOCKS;

    void *blocks[MAX_FRAG_BLOCKS];
    unsigned long i;

    for (i = 0; i < count; i++)
        blocks[i] = NULL;

    unsigned long t0_ms = now_ms();
    unsigned long long t0 = rdtsc64();

    for (i = 0; i < count; i++)
    {
        blocks[i] = malloc((unsigned int)size);
        if (!blocks[i])
        {
            r.failures++;
            break;
        }

        *(volatile char *)blocks[i] = 0;
        r.iterations++;
        r.payload_bytes += size;
    }

    for (i = 0; i < count; i++)
    {
        if (i % 2 == 0 && blocks[i])
        {
            free(blocks[i]);
            blocks[i] = NULL;
        }
    }

    for (i = 0; i < count; i++)
    {
        if (blocks[i])
            continue;

        blocks[i] = malloc((unsigned int)size);
        if (!blocks[i])
        {
            r.failures++;
            break;
        }

        *(volatile char *)blocks[i] = 0;
        r.iterations++;
        r.payload_bytes += size;
    }

    r.elapsed_cycles = rdtsc64() - t0;
    r.elapsed_ms = now_ms() - t0_ms;

    for (i = 0; i < count; i++)
    {
        if (blocks[i])
            free(blocks[i]);
    }

    collect_heap_stats(&r);
    print_result(&r);
}

static void bench_peak(unsigned long size)
{
    bench_result_t r;
    r.name = "peak";
    r.iterations = 0;
    r.failures = 0;
    r.payload_bytes = 0;

    void **blocks = NULL;
    unsigned long cap = 64;
    unsigned long nblocks = 0;

    blocks = (void **)malloc((unsigned int)(cap * sizeof(void *)));
    if (!blocks)
    {
        printf("\033[31mError: Failed to allocate tracking array.\033[0m\n");
        return;
    }

    unsigned long t0_ms = now_ms();
    unsigned long long t0 = rdtsc64();

    while (1)
    {
        void *p = malloc((unsigned int)size);
        if (!p)
        {
            r.failures = 1;
            break;
        }

        *(volatile char *)p = 0;

        if (nblocks >= cap)
        {
            cap *= 2;
            void **nb = (void **)realloc(blocks, (unsigned int)(cap * sizeof(void *)));
            if (!nb)
            {
                free(p);
                r.failures++;
                break;
            }
            blocks = nb;
        }

        blocks[nblocks++] = p;
        r.iterations++;
        r.payload_bytes += size;
    }

    r.elapsed_cycles = rdtsc64() - t0;
    r.elapsed_ms = now_ms() - t0_ms;

    for (unsigned long i = 0; i < nblocks; i++)
        free(blocks[i]);
    free(blocks);

    collect_heap_stats(&r);
    print_result(&r);
}

static void print_help(void)
{
    printf(
        "\033[1;33mbenchmem\033[0m - memory allocator benchmarks for thesis comparison\n\n"
        "Usage:\n"
        "  benchmem expand [count] [size]   Sequential malloc (default: 100 4096)\n"
        "  benchmem churn  [count] [size]   Malloc/free loop   (default: 500 256)\n"
        "  benchmem frag   [count] [size]   Fragmentation test (default: 200 128)\n"
        "  benchmem peak   [size]           Grow until OOM     (default: 4096)\n"
        "  benchmem all                     Run all scenarios\n"
        "  benchmem help                    Show this help\n\n");
}

static void bench_all(void)
{
    bench_expand(100, 4096);
    bench_churn(500, 256);
    bench_frag(200, 128);
    bench_peak(4096);
}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "help") == 0)
    {
        print_help();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "expand") == 0)
    {
        unsigned long count = parse_ulong(argc > 2 ? argv[2] : NULL, 100);
        unsigned long size = parse_ulong(argc > 3 ? argv[3] : NULL, 4096);
        bench_expand(count, size);
        return 0;
    }

    if (strcmp(cmd, "churn") == 0)
    {
        unsigned long count = parse_ulong(argc > 2 ? argv[2] : NULL, 500);
        unsigned long size = parse_ulong(argc > 3 ? argv[3] : NULL, 256);
        bench_churn(count, size);
        return 0;
    }

    if (strcmp(cmd, "frag") == 0)
    {
        unsigned long count = parse_ulong(argc > 2 ? argv[2] : NULL, 200);
        unsigned long size = parse_ulong(argc > 3 ? argv[3] : NULL, 128);
        bench_frag(count, size);
        return 0;
    }

    if (strcmp(cmd, "peak") == 0)
    {
        unsigned long size = parse_ulong(argc > 2 ? argv[2] : NULL, 4096);
        bench_peak(size);
        return 0;
    }

    if (strcmp(cmd, "all") == 0)
    {
        bench_all();
        return 0;
    }

    printf("\033[31mError: Unknown scenario '%s'. Try 'benchmem help'.\033[0m\n", cmd);
    return 1;
}
