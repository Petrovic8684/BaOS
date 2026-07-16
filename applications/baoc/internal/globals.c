#include "baoc_internal.h"

GlobalSym globals[MAX_GLOBALS];
int globals_cnt;
uint32_t data_size;
uint32_t bss_size;

static int global_find_local(const char *name)
{
    for (int i = 0; i < globals_cnt; i++)
    {
        if (strcmp(globals[i].name, name) == 0 && !globals[i].is_extern)
            return i;
    }
    return -1;
}

int global_find(const char *name)
{
    for (int i = 0; i < globals_cnt; i++)
    {
        if (strcmp(globals[i].name, name) == 0)
            return i;
    }
    return -1;
}

int global_find_static(const char *name)
{
    for (int i = 0; i < globals_cnt; i++)
    {
        if (strcmp(globals[i].name, name) == 0 && globals[i].is_static)
            return i;
    }
    return -1;
}

static uint32_t alloc_data_slot(int size, int is_bss)
{
    if (size < 4)
        size = 4;
    if (size % 4)
        size += 4 - (size % 4);

    if (is_bss)
    {
        uint32_t off = bss_size;
        bss_size += (uint32_t)size;
        return off;
    }

    uint32_t off = data_size;
    data_size += (uint32_t)size;
    return off;
}

int add_global(const char *name, const TypeSpec *ts, int is_static, int is_extern, int is_const,
               int has_init, int32_t init_val)
{
    if (globals_cnt >= MAX_GLOBALS)
        die("too many globals");

    int existing = -1;
    if (is_static)
        existing = global_find_static(name);
    else
        existing = global_find_local(name);

    if (existing >= 0 && !is_extern)
        die("redefinition of global variable");

    if (is_extern)
    {
        if (existing >= 0)
            return existing;
        int gi = globals_cnt++;
        memset(&globals[gi], 0, sizeof(globals[gi]));
        strncpy(globals[gi].name, name, sizeof(globals[gi].name) - 1);
        globals[gi].is_extern = 1;
        globals[gi].is_static = is_static ? 1 : 0;
        globals[gi].type = *ts;
        globals[gi].data_offset = 0xFFFFFFFFu;
        return gi;
    }

    int gi = globals_cnt++;
    memset(&globals[gi], 0, sizeof(globals[gi]));
    strncpy(globals[gi].name, name, sizeof(globals[gi].name) - 1);
    globals[gi].is_static = is_static ? 1 : 0;
    globals[gi].is_const = is_const ? 1 : 0;
    globals[gi].type = *ts;

    int size = type_spec_size(ts);
    int is_bss = !has_init;
    globals[gi].data_offset = alloc_data_slot(size, is_bss);
    globals[gi].has_init = has_init ? 1 : 0;
    globals[gi].init_val = init_val;
    return gi;
}

void globals_reset_sizes(void)
{
    data_size = 0;
    bss_size = 0;
}

static uint32_t baoc_str_pool_size(void)
{
    uint32_t total = 0;
    for (int i = 0; i < strlits_cnt; ++i)
        total += (uint32_t)(strlen(strlits[i].s) + 1);
    return total;
}

uint32_t global_vaddr(int gi)
{
    if (gi < 0 || gi >= globals_cnt)
        return 0;

    if (link_layout.data_vaddr != 0)
        return link_layout.data_vaddr + globals[gi].data_offset;

    return LOAD_BASE + (uint32_t)crt_sz + (uint32_t)libc_sz + (uint32_t)code_len +
           baoc_str_pool_size() + globals[gi].data_offset;
}

void emit_mov_eax_from_abs(uint32_t addr)
{
    emit_u8(0xA1);
    emit_u32(addr);
}

void emit_mov_abs_from_eax(uint32_t addr)
{
    emit_u8(0xA3);
    emit_u32(addr);
}

void emit_lea_eax_abs(uint32_t addr)
{
    emit_u8(0xB8);
    emit_u32(addr);
}

void emit_load_global_to_stack(int gi)
{
    uint32_t addr = global_vaddr(gi);
    if (globals[gi].type.ptr_depth > 0)
        emit_lea_eax_abs(addr);
    else
        emit_mov_eax_from_abs(addr);
    emit_push_eax();
}

void emit_store_global_from_eax(int gi)
{
    emit_mov_abs_from_eax(global_vaddr(gi));
}

int static_local_global_index(const char *vname)
{
    if (!current_func_name[0])
        return -1;
    char mname[80];
    snprintf(mname, sizeof(mname), "__%s_%s", current_func_name, vname);
    return global_find(mname);
}
