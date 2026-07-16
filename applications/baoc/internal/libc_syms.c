#include "baoc_internal.h"

#define BSYM_MAGIC 0x4D595342u /* 'BSYM' little-endian */

typedef struct
{
    char name[64];
    uint32_t offset;
} LibcSymEntry;

static LibcSymEntry libc_syms[MAX_LIBC_SYMS];
static int libc_syms_cnt;
static uint32_t libc_text_size;
static int libc_syms_loaded;

int baoc_libc_syms_load(const unsigned char *buf, size_t sz)
{
    libc_syms_cnt = 0;
    libc_text_size = 0;
    libc_syms_loaded = 0;

    if (!buf || sz < 12)
        return -1;

    uint32_t magic;
    memcpy(&magic, buf, 4);
    if (magic != BSYM_MAGIC)
        return -1;

    uint32_t lsize, count;
    memcpy(&lsize, buf + 4, 4);
    memcpy(&count, buf + 8, 4);
    libc_text_size = lsize;

    size_t pos = 12;
    for (uint32_t i = 0; i < count && libc_syms_cnt < MAX_LIBC_SYMS; i++)
    {
        if (pos >= sz)
            break;
        uint8_t nlen = buf[pos++];
        if (pos + nlen + 4 > sz)
            break;
        LibcSymEntry *e = &libc_syms[libc_syms_cnt++];
        memset(e, 0, sizeof(*e));
        size_t copy = nlen < 63 ? nlen : 63;
        memcpy(e->name, buf + pos, copy);
        e->name[copy] = '\0';
        pos += nlen;
        memcpy(&e->offset, buf + pos, 4);
        pos += 4;
    }

    libc_syms_loaded = (libc_syms_cnt > 0) ? 1 : 0;
    return libc_syms_loaded ? 0 : -1;
}

int baoc_libc_sym_find(const char *name, uint32_t *out_off)
{
    if (!name || !out_off)
        return -1;
    for (int i = 0; i < libc_syms_cnt; i++)
    {
        if (strcmp(libc_syms[i].name, name) == 0)
        {
            *out_off = libc_syms[i].offset;
            return 0;
        }
    }
    return -1;
}

uint32_t baoc_libc_text_size(void) { return libc_text_size; }

int baoc_libc_syms_ready(void) { return libc_syms_loaded; }
