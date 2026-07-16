#include "baoc_internal.h"

int struct_find(const char *name)
{
    if (!name)
        return -1;
    for (int i = 0; i < structsyms_cnt; ++i)
        if (strcmp(structsyms[i].name, name) == 0)
            return i;
    return -1;
}


int add_structsym(const char *name, int is_union)
{
    if (structsyms_cnt >= MAX_STRUCTS)
        die("too many structs/unions");
    strncpy(structsyms[structsyms_cnt].name, name, sizeof(structsyms[structsyms_cnt].name) - 1);
    structsyms[structsyms_cnt].name[sizeof(structsyms[structsyms_cnt].name) - 1] = '\0';
    structsyms[structsyms_cnt].is_union = is_union ? 1 : 0;
    structsyms[structsyms_cnt].field_cnt = 0;
    structsyms[structsyms_cnt].size = 0;
    return structsyms_cnt++;
}


int add_struct_field(int si, const char *fname, int field_size)
{
    if (si < 0 || si >= structsyms_cnt)
        die("internal: bad struct index");
    StructSym *s = &structsyms[si];
    if (s->field_cnt >= MAX_STRUCT_FIELDS)
        die("too many struct fields");
    int idx = s->field_cnt++;
    strncpy(s->field_names[idx], fname, sizeof(s->field_names[idx]) - 1);
    s->field_names[idx][sizeof(s->field_names[idx]) - 1] = '\0';
    if (field_size <= 0)
        field_size = 4;
    if (field_size % 4)
        field_size += (4 - (field_size % 4));
    s->field_sizes[idx] = field_size;
    if (s->is_union)
    {
        s->field_offsets[idx] = 0;
        if (field_size > s->size)
            s->size = field_size;
    }
    else
    {
        s->field_offsets[idx] = s->size;
        s->size += field_size;
    }
    return idx;
}


int struct_field_offset(int si, const char *fname)
{
    if (si < 0 || si >= structsyms_cnt)
        return -1;
    StructSym *s = &structsyms[si];
    for (int i = 0; i < s->field_cnt; i++)
    {
        if (strcmp(s->field_names[i], fname) == 0)
            return s->field_offsets[i];
    }
    return -1;
}


int struct_field_size(int si, const char *fname)
{
    if (si < 0 || si >= structsyms_cnt)
        return -1;
    StructSym *s = &structsyms[si];
    for (int i = 0; i < s->field_cnt; i++)
    {
        if (strcmp(s->field_names[i], fname) == 0)
            return s->field_sizes[i];
    }
    return -1;
}


int is_typedef_name(const char *name)
{
    for (int i = 0; i < typedefs_cnt; ++i)
        if (strcmp(typedefs[i].name, name) == 0)
            return 1;
    return 0;
}


void add_typedef_name(const char *name, int ptr_depth, int struct_id)
{
    if (typedefs_cnt >= MAX_TYPEDEFS)
        die("too many typedefs");
    strncpy(typedefs[typedefs_cnt].name, name, sizeof(typedefs[typedefs_cnt].name) - 1);
    typedefs[typedefs_cnt].name[sizeof(typedefs[typedefs_cnt].name) - 1] = '\0';
    typedefs[typedefs_cnt].ptr_depth = ptr_depth;
    typedefs[typedefs_cnt].struct_id = struct_id;
    typedefs_cnt++;
}


int lookup_typedef(const char *name, int *out_ptr_depth, int *out_struct_id)
{
    for (int i = 0; i < typedefs_cnt; ++i)
    {
        if (strcmp(typedefs[i].name, name) == 0)
        {
            if (out_ptr_depth)
                *out_ptr_depth = typedefs[i].ptr_depth;
            if (out_struct_id)
                *out_struct_id = typedefs[i].struct_id;
            return 1;
        }
    }
    return 0;
}


int enum_find(const char *name)
{
    for (int i = 0; i < enum_consts_cnt; ++i)
        if (strcmp(enum_consts[i].name, name) == 0)
            return i;
    return -1;
}


void add_enum_const(const char *name, int32_t val)
{
    if (enum_consts_cnt >= MAX_ENUM_CONSTS)
        die("too many enum constants");

    strncpy(enum_consts[enum_consts_cnt].name, name, sizeof(enum_consts[enum_consts_cnt].name) - 1);
    enum_consts[enum_consts_cnt].name[sizeof(enum_consts[enum_consts_cnt].name) - 1] = '\0';
    enum_consts[enum_consts_cnt].val = val;
    enum_consts_cnt++;
}


void add_string_literal_entry(const char *s)
{
    if (strlits_cnt >= MAX_STR_LITS)
        die("too many string literals");

    size_t len = strlen(s) + 1;
    if (str_storage_used + len > STR_STORAGE_SIZE)
        die("string storage overflow");

    char *dst = &str_storage[str_storage_used];
    for (size_t i = 0; i < len; i++)
        dst[i] = s[i];

    strlits[strlits_cnt].s = dst;

    if (code_len < 4)
        die("internal: code too small for string immediate");

    strlits[strlits_cnt].code_place = (uint32_t)(code_len - 4);
    strlits_cnt++;

    str_storage_used += len;
}


int is_type_name(const char *name)
{
    if (!name)
        return 0;

    if (strcmp(name, "int") == 0)
        return 1;
    if (strcmp(name, "void") == 0)
        return 1;
    if (strcmp(name, "char") == 0)
        return 1;
    if (strcmp(name, "unsigned") == 0)
        return 1;
    if (strcmp(name, "signed") == 0)
        return 1;
    if (strcmp(name, "short") == 0)
        return 1;
    if (strcmp(name, "long") == 0)
        return 1;
    if (strcmp(name, "float") == 0)
        return 1;
    if (strcmp(name, "double") == 0)
        return 1;

    if (lookup_typedef(name, NULL, NULL))
        return 1;

    return 0;
}


int func_find(const char *name)
{
    for (int i = 0; i < funcs_cnt; i++)
        if (strcmp(funcs[i].name, name) == 0)
            return i;
    return -1;
}


void add_reloc(const char *name, uint32_t call_site)
{
    if (relocs_cnt >= MAX_RELOCS)
        die("too many relocs");

    strncpy(relocs[relocs_cnt].name, name, sizeof(relocs[relocs_cnt].name) - 1);
    relocs[relocs_cnt].name[sizeof(relocs[relocs_cnt].name) - 1] = '\0';
    relocs[relocs_cnt].sym_name[0] = '\0';
    relocs[relocs_cnt].call_site = call_site;
    relocs_cnt++;
}

void add_reloc_extern(const char *name, uint32_t call_site)
{
    add_reloc(name, call_site);
    strncpy(relocs[relocs_cnt - 1].sym_name, name, sizeof(relocs[relocs_cnt - 1].sym_name) - 1);
    relocs[relocs_cnt - 1].sym_name[sizeof(relocs[relocs_cnt - 1].sym_name) - 1] = '\0';
}


int local_find(const char *name)
{
    for (int i = 0; i < locals_cnt; i++)
        if (strcmp(locals[i].name, name) == 0)
            return i;
    return -1;
}


int begin_local_slot(const char *name, int is_const)
{
    if (locals_cnt >= MAX_LOCALS)
        die("too many locals");

    int li = locals_cnt++;
    locals[li].ebp_disp = 0;
    strncpy(locals[li].name, name, sizeof(locals[li].name) - 1);
    locals[li].name[sizeof(locals[li].name) - 1] = '\0';
    locals[li].is_const = is_const ? 1 : 0;
    locals[li].ptr_depth = 0;
    locals[li].arr_size = 0;
    locals[li].struct_id = -1;
    return li;
}


void assign_local_frame(int li, int size)
{
    if (size < 4)
        size = 4;

    local_stack_size += size;
    locals[li].ebp_disp = -(int32_t)local_stack_size;
}

void finish_local_alloc(int li, int size)
{
    if (size < 4)
        size = 4;

    local_stack_size += size;
    locals[li].ebp_disp = -(int32_t)local_stack_size;
    emit_sub_esp_imm((uint32_t)size);

    for (int off = 0; off < size; off += 4)
    {
        int32_t disp = locals[li].ebp_disp + off;
        emit_u8(0xC7);
        if (disp >= -128 && disp <= 127)
        {
            emit_u8(0x45);
            emit_u8((uint8_t)disp);
        }
        else
        {
            emit_u8(0x85);
            emit_u32((uint32_t)disp);
        }
        emit_u32(0);
    }
}
