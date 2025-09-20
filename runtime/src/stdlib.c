#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#define SYS_EXIT 0
#define SYS_SET_USER_PAGES 21

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALLOC_ALIGN 8
#define PAGE_SIZE_LOCAL 4096

typedef struct free_hdr
{
    unsigned int size;
    struct free_hdr *next;
} free_hdr_t;

typedef struct alloc_hdr
{
    unsigned int size;
} alloc_hdr_t;

static free_hdr_t *free_list = NULL;
extern unsigned int _end;

static unsigned int heap_start = 0;
static unsigned int heap_end = 0;
static unsigned int heap_max = 0;

#define USER_STACK_TOP 0x02100000U
#define USER_STACK_PAGES 4U

void exit(int code)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[c], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_EXIT), [c] "r"(code)
        : "eax", "ebx", "memory");
}

double strtod(const char *nptr, char **endptr)
{
    const char *s = nptr;
    double result = 0.0;
    int sign = 1;
    int exp_sign = 1;
    long exponent = 0;
    double frac = 0.0;
    double frac_div = 1.0;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '+')
        s++;
    else if (*s == '-')
    {
        sign = -1;
        s++;
    }

    while (isdigit((unsigned char)*s))
    {
        result = result * 10.0 + (*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;
        while (isdigit((unsigned char)*s))
        {
            frac = frac * 10.0 + (*s - '0');
            frac_div *= 10.0;
            s++;
        }
        result += frac / frac_div;
    }

    if (*s == 'e' || *s == 'E')
    {
        s++;
        if (*s == '+')
            s++;
        else if (*s == '-')
        {
            exp_sign = -1;
            s++;
        }
        while (isdigit((unsigned char)*s))
        {
            exponent = exponent * 10 + (*s - '0');
            s++;
        }
    }

    if (exponent != 0)
    {
        double pow10 = pow(10.0, exp_sign * exponent);
        result *= pow10;
    }

    if (endptr != NULL)
        *endptr = (char *)s;

    return sign * result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    unsigned long acc = 0;
    int c;
    int neg = 0;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '+')
        s++;

    else if (*s == '-')
    {
        neg = 1;
        s++;
    }

    if (base == 0)
    {
        if (*s == '0')
        {
            if (s[1] == 'x' || s[1] == 'X')
            {
                base = 16;
                s += 2;
            }
            else
            {
                base = 8;
                s++;
            }
        }
        else
            base = 10;
    }
    else if (base == 16)
    {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    const char *start = s;

    while ((c = (unsigned char)*s))
    {
        unsigned int digit;

        if (isdigit(c))
            digit = c - '0';
        else if (isalpha(c))
            digit = (tolower(c) - 'a') + 10;
        else
            break;

        if (digit >= (unsigned int)base)
            break;

        if (acc > (ULONG_MAX - digit) / base)
        {
            acc = ULONG_MAX;
            while (isdigit((unsigned char)*s) || isalpha((unsigned char)*s))
                s++;
            break;
        }

        acc = acc * base + digit;
        s++;
    }

    if (endptr != NULL)
        *endptr = (char *)(s != start ? s : nptr);

    return neg ? (unsigned long)(-(long)acc) : acc;
}

long strtol(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    int neg = 0;
    unsigned long acc = 0;
    int any = 0;
    unsigned int c;
    unsigned long cutoff;
    unsigned int cutlim;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '+')
        s++;
    else if (*s == '-')
    {
        neg = 1;
        s++;
    }

    if (base == 0)
    {
        if (*s == '0')
        {
            if (s[1] == 'x' || s[1] == 'X')
            {
                base = 16;
                s += 2;
            }
            else
            {
                base = 8;
                s++;
            }
        }
        else
            base = 10;
    }
    else if (base == 16)
    {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    const char *start = s;

    if (neg)
    {
        cutoff = (unsigned long)LONG_MAX + 1UL;
    }
    else
    {
        cutoff = (unsigned long)LONG_MAX;
    }
    cutlim = (unsigned int)(cutoff % (unsigned long)base);
    unsigned long cutval = cutoff / (unsigned long)base;

    while ((c = (unsigned char)*s))
    {
        unsigned int digit;
        if (isdigit(c))
            digit = c - '0';
        else if (isalpha(c))
            digit = (unsigned int)(tolower(c) - 'a') + 10;
        else
            break;

        if (digit >= (unsigned int)base)
            break;

        if (acc > cutval || (acc == cutval && digit > cutlim))
        {
            any = 1;
            acc = (unsigned long)LONG_MAX + (neg ? 1UL : 0UL);
            while (isdigit((unsigned char)*s) || isalpha((unsigned char)*s))
                s++;
            break;
        }

        any = 1;
        acc = acc * (unsigned long)base + (unsigned long)digit;
        s++;
    }

    if (endptr != NULL)
        *endptr = (char *)(any ? s : nptr);

    if (!any)
        return 0;

    if (acc == (unsigned long)LONG_MAX + 1UL && neg)
        return LONG_MIN;
    if (acc > (unsigned long)LONG_MAX && !neg)
        return LONG_MAX;

    return neg ? -(long)acc : (long)acc;
}

static void sys_set_user_pages(unsigned int virt_start, unsigned int size)
{
    struct map_args
    {
        unsigned int virt_start;
        unsigned int size;
    } args;
    args.virt_start = virt_start;
    args.size = size;

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_SET_USER_PAGES), [arg] "r"(&args)
        : "eax", "ebx", "memory");
}

static void *heap_expand(unsigned int bytes)
{
    if (bytes == 0)
        return (void *)heap_end;

    unsigned int need_end = ALIGN_UP(heap_end + bytes, PAGE_SIZE_LOCAL);

    if (need_end > heap_max)
        return NULL;

    unsigned int map_start = heap_end;
    unsigned int map_size = need_end - heap_end;

    sys_set_user_pages(map_start, map_size);

    void *old = (void *)heap_end;
    heap_end = need_end;
    return old;
}

static void free_list_insert_and_coalesce(free_hdr_t *blk)
{
    free_hdr_t **p = &free_list;
    while (*p && (unsigned int)(*p) < (unsigned int)blk)
        p = &(*p)->next;

    blk->next = *p;
    *p = blk;

    if (blk->next)
    {
        unsigned int blk_end = (unsigned int)blk + blk->size;
        if (blk_end == (unsigned int)blk->next)
        {
            blk->size += blk->next->size;
            blk->next = blk->next->next;
        }
    }

    if (p != &free_list)
    {
        free_hdr_t *prev = free_list;
        while (prev && prev->next != blk)
            prev = prev->next;

        if (prev)
        {
            unsigned int prev_end = (unsigned int)prev + prev->size;
            if (prev_end == (unsigned int)blk)
            {
                prev->size += blk->size;
                prev->next = blk->next;
            }
        }
    }
}

static void heap_init_once(void)
{
    if (heap_start != 0)
        return;

    heap_start = ALIGN_UP((unsigned int)&_end, PAGE_SIZE_LOCAL);
    heap_end = heap_start;

    heap_max = USER_STACK_TOP - (USER_STACK_PAGES * PAGE_SIZE_LOCAL);

    free_list = NULL;
}

void *malloc(unsigned int size)
{
    if (size == 0)
        return NULL;

    heap_init_once();

    unsigned int payload_sz = (unsigned int)ALIGN_UP(size, ALLOC_ALIGN);
    unsigned int total_sz = payload_sz + sizeof(alloc_hdr_t);

    free_hdr_t **p = &free_list;
    while (*p)
    {
        if ((*p)->size >= total_sz)
        {
            free_hdr_t *found = *p;

            if (found->size >= total_sz + (unsigned int)sizeof(free_hdr_t) + ALLOC_ALIGN)
            {
                unsigned int orig_size = found->size;
                unsigned int leftover_size = orig_size - total_sz;
                unsigned int alloc_addr = (unsigned int)found;
                unsigned int new_free_addr = alloc_addr + total_sz;

                free_hdr_t *new_free = (free_hdr_t *)new_free_addr;
                new_free->size = leftover_size;
                new_free->next = found->next;

                *p = new_free;

                alloc_hdr_t *ah = (alloc_hdr_t *)alloc_addr;
                ah->size = total_sz;
                return (void *)(alloc_addr + sizeof(alloc_hdr_t));
            }
            else
            {
                *p = found->next;
                alloc_hdr_t *ah = (alloc_hdr_t *)found;
                ah->size = found->size;
                return (void *)((unsigned int)found + sizeof(alloc_hdr_t));
            }
        }
        p = &(*p)->next;
    }

    void *addr = heap_expand(total_sz);
    if (!addr)
        return NULL;

    alloc_hdr_t *ah = (alloc_hdr_t *)addr;
    ah->size = total_sz;
    return (void *)((unsigned int)addr + sizeof(alloc_hdr_t));
}

void free(void *ptr)
{
    if (!ptr)
        return;

    alloc_hdr_t *ah = (alloc_hdr_t *)((unsigned int)ptr - sizeof(alloc_hdr_t));
    free_hdr_t *blk = (free_hdr_t *)ah;
    blk->size = ah->size;

    free_list_insert_and_coalesce(blk);
}

void *realloc(void *ptr, unsigned int size)
{
    if (ptr == NULL)
        return malloc(size);

    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    alloc_hdr_t *ah = (alloc_hdr_t *)((unsigned int)ptr - sizeof(alloc_hdr_t));
    unsigned int old_total = ah->size;
    unsigned int old_payload = old_total - sizeof(alloc_hdr_t);

    if (old_payload >= size)
        return ptr;

    void *newp = malloc(size);
    if (!newp)
        return NULL;

    unsigned int to_copy = (old_payload < size) ? old_payload : size;
    memcpy(newp, ptr, to_copy);
    free(ptr);
    return newp;
}

void *calloc(unsigned int nmemb, unsigned int size)
{
    if (nmemb == 0 || size == 0)
        return malloc(0);

    if (size != 0 && nmemb > (unsigned int)(~0u) / size)
        return NULL;

    unsigned int total = nmemb * size;
    void *p = malloc(total);
    if (!p)
        return NULL;

    memset(p, 0, total);
    return p;
}