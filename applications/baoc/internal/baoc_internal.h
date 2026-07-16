#ifndef BAOC_INTERNAL_H
#define BAOC_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>

#define LOAD_BASE 0x01000000
#define MAX_FILE_SIZE (1024 * 64)
#define MAX_IMAGE (MAX_FILE_SIZE * 8)
#define MAX_CODE (1024 * 64)
#define MAX_RELOCS 2048
#define MAX_FUNCS 512
#define MAX_LOCALS 256
#define MAX_GLOBALS 256
#define MAX_LIBC_SYMS 512
#define MAX_LOOP_DEPTH 64
#define MAX_SWITCH_DEPTH 32
#define MAX_LABELS 128
#define MAX_TYPEDEFS 64
#define MAX_ENUM_CONSTS 512
#define MAX_STR_LITS 512
#define STR_STORAGE_SIZE 65536
#define MAX_STRUCTS 64
#define MAX_STRUCT_FIELDS 64
#define MAX_SOURCE_FILES 16
#define PP_MAX_DEPTH 32
#define PP_MAX_MACROS 256
#define PP_MAX_MACRO_NAME 64
#define PP_MAX_MACRO_BODY 1536
#define PP_MAX_MACRO_PARAMS 8
#define PP_MAX_MACRO_PARAM_LEN 64
#define PP_FILE_BUF_SIZE 65536
#define PP_OUT_MAX (PP_FILE_BUF_SIZE * 2)
#define PP_LINEBUF 4096
#define PP_ARG_MAX 16
#define PP_ARG_LEN 1024

#define OP_EQ 1
#define OP_NE 2
#define OP_LE 3
#define OP_GE 4
#define OP_LT 6
#define OP_GT 7
#define OP_LAND 8
#define OP_LOR 9
#define OP_LNOT 10
#define OP_BAND 11
#define OP_BOR 12
#define OP_BXOR 13
#define OP_SHL 14
#define OP_SHR 15
#define OP_INC 16
#define OP_DEC 17
#define OP_ARROW 18
#define OP_COMPL 19

#define TY_VOID 0
#define TY_CHAR 1
#define TY_SHORT 2
#define TY_INT 3
#define TY_LONG 4
#define TY_FLOAT 5
#define TY_DOUBLE 6
#define TY_STRUCT 7

typedef enum
{
    TOK_EOF,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_SYM,
    TOK_OP
} TokType;

typedef struct
{
    TokType type;
    char ident[64];
    int32_t val;
    char sym;
    int op;
    char str[256];
} Token;

typedef struct
{
    int base;
    int size;
    int unsigned_flag;
    int is_const;
    int ptr_depth;
    int struct_id;
} TypeSpec;

typedef struct
{
    char name[64];
    uint32_t call_site;
    char sym_name[64];
} Reloc;

typedef struct
{
    char name[64];
    uint32_t offset;
    int argc;
    int is_variadic;
    int is_extern;
    int is_libc;
    int ret_ptr_depth;
    int ret_struct_id;
    TypeSpec ret_type;
    int param_ptr_depth[16];
    int param_struct_id[16];
} FuncSym;

typedef struct
{
    char name[64];
    int32_t ebp_disp;
    int is_const;
    int is_static_local;
    int ptr_depth;
    int arr_size;
    int struct_id;
    int elem_size;
    int type_base;
} LocalVar;

typedef struct
{
    char name[64];
    uint32_t data_offset;
    int is_static;
    int is_extern;
    int is_const;
    int has_init;
    int32_t init_val;
    TypeSpec type;
} GlobalSym;

typedef struct
{
    char name[64];
    int ptr_depth;
    int struct_id;
} TypedefSym;

typedef struct
{
    char name[64];
    int32_t val;
} EnumConst;

typedef struct
{
    char *s;
    uint32_t code_place;
} StrLit;

typedef struct
{
    char name[64];
    int is_union;
    int field_cnt;
    char field_names[MAX_STRUCT_FIELDS][64];
    int field_offsets[MAX_STRUCT_FIELDS];
    int field_sizes[MAX_STRUCT_FIELDS];
    int size;
} StructSym;

typedef struct
{
    char name[64];
    size_t code_addr;
} LabelSym;

typedef struct
{
    unsigned char used;
    unsigned char is_function;
    unsigned char param_count;
    char name[PP_MAX_MACRO_NAME];
    char body[PP_MAX_MACRO_BODY];
    char params[PP_MAX_MACRO_PARAMS][PP_MAX_MACRO_PARAM_LEN];
} PP_Macro;

typedef struct
{
    uint32_t crt_size;
    uint32_t libc_size;
    uint32_t user_text_off;
    uint32_t user_text_vaddr;
    uint32_t rodata_off;
    uint32_t rodata_vaddr;
    uint32_t data_off;
    uint32_t data_vaddr;
    uint32_t bss_off;
    uint32_t bss_vaddr;
    uint32_t total_filesz;
    uint32_t total_memsz;
} LinkLayout;

extern size_t break_addrs[MAX_LOOP_DEPTH][128];
extern int break_counts[MAX_LOOP_DEPTH];
extern size_t continue_addrs[MAX_LOOP_DEPTH][128];
extern int continue_counts[MAX_LOOP_DEPTH];
extern size_t loop_start[MAX_LOOP_DEPTH];
extern size_t loop_cond[MAX_LOOP_DEPTH];
extern int loop_depth;

extern size_t switch_break_addrs[MAX_SWITCH_DEPTH][64];
extern int switch_break_counts[MAX_SWITCH_DEPTH];
extern int switch_depth;

extern unsigned char *src_buf;
extern unsigned char *crt_buf;
extern unsigned char *libc_buf;
extern unsigned char *syms_buf;
extern unsigned char *image_buf;
extern unsigned char *code_buf;
extern unsigned char *preproc_buf;
extern size_t code_len;
extern size_t crt_sz;
extern size_t libc_sz;
extern size_t syms_sz;
extern int current_fun_argbytes;
extern int current_fun_saw_return;
extern int in_cast_expr;
extern char current_func_name[64];

extern Reloc relocs[MAX_RELOCS];
extern int relocs_cnt;
extern FuncSym funcs[MAX_FUNCS];
extern int funcs_cnt;
extern LocalVar locals[MAX_LOCALS];
extern int locals_cnt;
extern int local_stack_size;
extern GlobalSym globals[MAX_GLOBALS];
extern int globals_cnt;
extern uint32_t data_size;
extern uint32_t bss_size;
extern TypedefSym typedefs[MAX_TYPEDEFS];
extern int typedefs_cnt;
extern EnumConst enum_consts[MAX_ENUM_CONSTS];
extern int enum_consts_cnt;
extern StrLit strlits[MAX_STR_LITS];
extern int strlits_cnt;
extern LabelSym labels[MAX_LABELS];
extern int labels_cnt;

extern char str_storage[STR_STORAGE_SIZE];
extern uint32_t str_storage_used;

extern Token curtok;
extern char *src;

extern char last_primary_ident[64];
extern int last_primary_is_simple;

extern StructSym structsyms[MAX_STRUCTS];
extern int structsyms_cnt;

extern LinkLayout link_layout;

extern PP_Macro pp_macros[PP_MAX_MACROS];

void die(const char *m);

int baoc_memory_init(void);
void baoc_memory_free(void);
void baoc_compile_reset(void);
void baoc_file_reset(void);

void preprocess_src(const unsigned char *in, unsigned char *out, size_t out_max);

int struct_find(const char *name);
int add_structsym(const char *name, int is_union);
int add_struct_field(int si, const char *fname, int field_size);
int struct_field_offset(int si, const char *fname);
int struct_field_size(int si, const char *fname);
int is_typedef_name(const char *name);
void add_typedef_name(const char *name, int ptr_depth, int struct_id);
int lookup_typedef(const char *name, int *out_ptr_depth, int *out_struct_id);
void add_enum_const(const char *name, int32_t val);
int is_type_name(const char *name);

void type_spec_init(TypeSpec *ts);
int parse_type_spec(TypeSpec *ts);
int type_spec_size(const TypeSpec *ts);
int type_spec_is_float(const TypeSpec *ts);
int sizeof_type_name(void);
void local_from_typespec(int li, const TypeSpec *ts);

int global_find(const char *name);
int global_find_static(const char *name);
int add_global(const char *name, const TypeSpec *ts, int is_static, int is_extern, int is_const,
               int has_init, int32_t init_val);
void globals_reset_sizes(void);
uint32_t global_vaddr(int gi);
void emit_mov_eax_from_abs(uint32_t addr);
void emit_mov_abs_from_eax(uint32_t addr);
void emit_lea_eax_abs(uint32_t addr);
void emit_load_global_to_stack(int gi);
void emit_store_global_from_eax(int gi);
int static_local_global_index(const char *vname);

int baoc_libc_syms_load(const unsigned char *buf, size_t sz);
int baoc_libc_sym_find(const char *name, uint32_t *out_off);
uint32_t baoc_libc_text_size(void);
int baoc_libc_syms_ready(void);

char *skip_ws(char *p);

int enum_find(const char *name);
void add_string_literal_entry(const char *s);

void next_token(void);
int accept_ident(const char *s);
int accept_sym(char c);
void expect_sym(char c);
void expect_ident(void);

void emit_u8(unsigned char b);
void emit_u16(uint16_t v);
void emit_u32(uint32_t v);
void emit_push_imm32(uint32_t v);
void emit_pop_eax(void);
void emit_pop_ebx(void);
void emit_push_eax(void);
void emit_mov_eax_from_ebp_disp_any(int32_t disp);
void emit_mov_ebp_disp_from_eax_any(int32_t disp);
void emit_lea_eax_ebp_disp_any(int32_t disp);
void emit_load_eax_from_eax_ptr(void);
void emit_load_eax_from_ebx_ptr(void);
void emit_mov_ebx_from_eax(void);
void emit_mov_ptr_ebx_from_eax(void);
void emit_shl_ebx_imm(uint8_t imm);
void emit_add_eax_ebx(void);
void emit_sub_eax_ebx(void);
void emit_imul_eax_ebx(void);
void emit_cdq(void);
void emit_idiv_ebx(void);
void emit_test_eax_eax(void);
size_t emit_jz_rel32_placeholder(void);
size_t emit_jnz_rel32_placeholder(void);
size_t emit_jmp_rel32_placeholder(void);
void patch_rel32_at(size_t place, uint32_t rel32);
size_t emit_call_rel32_placeholder(void);
void emit_leave(void);
void emit_ret_pop_args_uint16(uint16_t argbytes);
void emit_sub_esp_imm(uint32_t imm);
void emit_setcc_and_push(uint8_t setcc_op);

int func_find(const char *name);
void add_reloc(const char *name, uint32_t call_site);
void add_reloc_extern(const char *name, uint32_t call_site);
int local_find(const char *name);
int begin_local_slot(const char *name, int is_const);
void assign_local_frame(int li, int size);
void finish_local_alloc(int li, int size);

static inline int32_t local_addr_disp(int li)
{
    return locals[li].ebp_disp;
}

int label_find(const char *name);
int label_define(const char *name);
size_t label_get_addr(int li);

void gen_call_args_cdecl(void);
void gen_emit_call_by_name(const char *name);
void gen_emit_call_eax_indirect(void);
void gen_assign(void);
void gen_expr(void);
void gen_cmp(void);
void gen_ternary(void);
void gen_stmt(void);
void gen_function(void);
void gen_global_decl(void);
void compile(const char *input);

int baoc_link_elf(const char *out_path);

#endif
