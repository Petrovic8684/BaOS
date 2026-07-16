#include "baoc_internal.h"

size_t break_addrs[MAX_LOOP_DEPTH][128];
int break_counts[MAX_LOOP_DEPTH];
size_t continue_addrs[MAX_LOOP_DEPTH][128];
int continue_counts[MAX_LOOP_DEPTH];
size_t loop_start[MAX_LOOP_DEPTH];
size_t loop_cond[MAX_LOOP_DEPTH];
int loop_depth = 0;

size_t switch_break_addrs[MAX_SWITCH_DEPTH][64];
int switch_break_counts[MAX_SWITCH_DEPTH];
int switch_depth = 0;

unsigned char *src_buf;
unsigned char *crt_buf;
unsigned char *libc_buf;
unsigned char *syms_buf;
unsigned char *image_buf;
unsigned char *code_buf;
unsigned char *preproc_buf;
size_t code_len = 0;
size_t crt_sz = 0;
size_t libc_sz = 0;
size_t syms_sz = 0;

int current_fun_argbytes = 0;
int current_fun_saw_return = 0;
int in_cast_expr = 0;
char current_func_name[64];

Reloc relocs[MAX_RELOCS];
int relocs_cnt = 0;

FuncSym funcs[MAX_FUNCS];
int funcs_cnt = 0;

LocalVar locals[MAX_LOCALS];
int locals_cnt;
int local_stack_size;

TypedefSym typedefs[MAX_TYPEDEFS];
int typedefs_cnt = 0;

EnumConst enum_consts[MAX_ENUM_CONSTS];
int enum_consts_cnt = 0;

StrLit strlits[MAX_STR_LITS];
int strlits_cnt = 0;

LabelSym labels[MAX_LABELS];
int labels_cnt = 0;

char str_storage[STR_STORAGE_SIZE];
uint32_t str_storage_used = 0;

Token curtok;
char *src;

char last_primary_ident[64];
int last_primary_is_simple = 0;

StructSym structsyms[MAX_STRUCTS];
int structsyms_cnt = 0;

LinkLayout link_layout;

PP_Macro pp_macros[PP_MAX_MACROS];

int baoc_memory_init(void)
{
    src_buf = (unsigned char *)malloc(MAX_FILE_SIZE);
    crt_buf = (unsigned char *)malloc(MAX_FILE_SIZE);
    libc_buf = (unsigned char *)malloc(MAX_FILE_SIZE);
    syms_buf = (unsigned char *)malloc(MAX_FILE_SIZE);
    image_buf = (unsigned char *)malloc(MAX_IMAGE);
    code_buf = (unsigned char *)malloc(MAX_CODE);
    preproc_buf = (unsigned char *)malloc(MAX_FILE_SIZE * 3);

    if (!src_buf || !crt_buf || !libc_buf || !syms_buf || !image_buf || !code_buf || !preproc_buf)
    {
        baoc_memory_free();
        return -1;
    }

    return 0;
}

void baoc_memory_free(void)
{
    free(src_buf);
    free(crt_buf);
    free(libc_buf);
    free(syms_buf);
    free(image_buf);
    free(code_buf);
    free(preproc_buf);
    src_buf = NULL;
    crt_buf = NULL;
    libc_buf = NULL;
    syms_buf = NULL;
    image_buf = NULL;
    code_buf = NULL;
    preproc_buf = NULL;
}

void baoc_compile_reset(void)
{
    code_len = 0;
    relocs_cnt = 0;
    funcs_cnt = 0;
    globals_cnt = 0;
    data_size = 0;
    bss_size = 0;
    strlits_cnt = 0;
    str_storage_used = 0;
    typedefs_cnt = 0;
    enum_consts_cnt = 0;
    structsyms_cnt = 0;
    labels_cnt = 0;
    loop_depth = 0;
    switch_depth = 0;
    last_primary_is_simple = 0;
    last_primary_ident[0] = '\0';
    current_fun_saw_return = 0;
    memset(&link_layout, 0, sizeof(link_layout));
}

void baoc_file_reset(void)
{
    loop_depth = 0;
    switch_depth = 0;
    last_primary_is_simple = 0;
    last_primary_ident[0] = '\0';
}

int label_find(const char *name)
{
    for (int i = 0; i < labels_cnt; i++)
        if (strcmp(labels[i].name, name) == 0)
            return i;
    return -1;
}

int label_define(const char *name)
{
    int li = label_find(name);
    if (li >= 0)
    {
        if (labels[li].code_addr != 0)
            die("duplicate label");
        labels[li].code_addr = code_len;
        return li;
    }
    if (labels_cnt >= MAX_LABELS)
        die("too many labels");
    strncpy(labels[labels_cnt].name, name, sizeof(labels[labels_cnt].name) - 1);
    labels[labels_cnt].name[sizeof(labels[labels_cnt].name) - 1] = '\0';
    labels[labels_cnt].code_addr = code_len;
    return labels_cnt++;
}

size_t label_get_addr(int li)
{
    if (li < 0 || li >= labels_cnt)
        return 0;
    return labels[li].code_addr;
}
