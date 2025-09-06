#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>

#define LOAD_BASE 0x100000
#define MAX_FILE_SIZE (1024 * 64)
#define MAX_IMAGE (MAX_FILE_SIZE * 6)
#define MAX_CODE (1024 * 64)
#define MAX_RELOCS 1024
#define MAX_FUNCS 256
#define MAX_LOCALS 256

#define MAX_LOOP_DEPTH 64
size_t break_addrs[MAX_LOOP_DEPTH][128];
int break_counts[MAX_LOOP_DEPTH];
size_t continue_addrs[MAX_LOOP_DEPTH][128];
int continue_counts[MAX_LOOP_DEPTH];
size_t loop_start[MAX_LOOP_DEPTH];
size_t loop_cond[MAX_LOOP_DEPTH];
size_t continue_target[MAX_LOOP_DEPTH];
int loop_depth = 0;

unsigned char src_buf[MAX_FILE_SIZE];
unsigned char crt_buf[MAX_FILE_SIZE];
unsigned char image_buf[MAX_IMAGE];
unsigned char code_buf[MAX_CODE];
size_t code_len = 0;

int current_fun_argbytes = 0;

typedef enum
{
    TOK_EOF,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_SYM,
    TOK_OP
} TokType;

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
    char name[64];
    uint32_t call_site;
} Reloc;
Reloc relocs[MAX_RELOCS];
int relocs_cnt = 0;

typedef struct
{
    char name[64];
    uint32_t offset;
    int argc;
} FuncSym;
FuncSym funcs[MAX_FUNCS];
int funcs_cnt = 0;

typedef struct
{
    char name[64];
    int offset;
    int is_const;
    int ptr_depth;
    int arr_size;
    int struct_id;
} LocalVar;

LocalVar locals[MAX_LOCALS];
int locals_cnt;
int local_stack_size;

#define MAX_TYPEDEFS 64

typedef struct
{
    char name[64];
} TypedefSym;

TypedefSym typedefs[MAX_TYPEDEFS];
int typedefs_cnt = 0;

#define MAX_ENUM_CONSTS 512
typedef struct
{
    char name[64];
    int32_t val;
} EnumConst;
EnumConst enum_consts[MAX_ENUM_CONSTS];
int enum_consts_cnt = 0;

#define MAX_STR_LITS 512
typedef struct
{
    char *s;
    uint32_t code_place;
} StrLit;
StrLit strlits[MAX_STR_LITS];
int strlits_cnt = 0;

#define STR_STORAGE_SIZE 65536

static char str_storage[STR_STORAGE_SIZE];
static uint32_t str_storage_used = 0;

Token curtok;
char *src;

static char last_primary_ident[64];
static int last_primary_is_simple = 0;

#define MAX_STRUCTS 64
#define MAX_STRUCT_FIELDS 64

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

StructSym structsyms[MAX_STRUCTS];
int structsyms_cnt = 0;

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

typedef struct
{
    unsigned char used;
    unsigned char is_function;
    unsigned char param_count;
    char name[PP_MAX_MACRO_NAME];
    char body[PP_MAX_MACRO_BODY];
    char params[PP_MAX_MACRO_PARAMS][PP_MAX_MACRO_PARAM_LEN];
} PP_Macro;

static PP_Macro pp_macros[PP_MAX_MACROS];

void die(const char *m)
{
    fprintf(stderr, "ERR: %s\n", m);
    fprintf(stderr, "Current token: type=%d ", curtok.type);
    if (curtok.type == TOK_IDENT)
        fprintf(stderr, "IDENT '%s'\n", curtok.ident);
    else if (curtok.type == TOK_NUMBER)
        fprintf(stderr, "NUMBER %d\n", curtok.val);
    else if (curtok.type == TOK_STRING)
        fprintf(stderr, "STRING \"%s\"\n", curtok.str);
    else if (curtok.type == TOK_SYM)
        fprintf(stderr, "SYM '%c'\n", curtok.sym);
    else if (curtok.type == TOK_OP)
    {
        const char *opname = "??";
        if (curtok.op == OP_EQ)
            opname = "==";
        else if (curtok.op == OP_NE)
            opname = "!=";
        else if (curtok.op == OP_LE)
            opname = "<=";
        else if (curtok.op == OP_GE)
            opname = ">=";
        fprintf(stderr, "OP %s\n", opname);
    }
    else
        fprintf(stderr, "EOF\n");
    if (src)
        fprintf(stderr, "Remaining source (around current pos): \"%s\"\n", src);
    exit(1);
}

static void pp_macros_clear_all(void)
{
    for (int i = 0; i < PP_MAX_MACROS; ++i)
    {
        pp_macros[i].used = 0;
        pp_macros[i].is_function = 0;
        pp_macros[i].param_count = 0;
        pp_macros[i].name[0] = '\0';
        pp_macros[i].body[0] = '\0';
    }
}

static PP_Macro *pp_macro_find(const char *name)
{
    for (int i = 0; i < PP_MAX_MACROS; ++i)
    {
        if (pp_macros[i].used && strcmp(pp_macros[i].name, name) == 0)
            return &pp_macros[i];
    }
    return NULL;
}

static PP_Macro *pp_macro_alloc(void)
{
    for (int i = 0; i < PP_MAX_MACROS; ++i)
    {
        if (!pp_macros[i].used)
        {
            pp_macros[i].used = 1;
            pp_macros[i].is_function = 0;
            pp_macros[i].param_count = 0;
            pp_macros[i].name[0] = '\0';
            pp_macros[i].body[0] = '\0';
            return &pp_macros[i];
        }
    }
    return NULL;
}

static void pp_macro_undef(const char *name)
{
    PP_Macro *m = pp_macro_find(name);
    if (m)
    {
        m->used = 0;
        m->name[0] = '\0';
        m->body[0] = '\0';
        m->param_count = 0;
    }
}

static void pp_macro_add_object(const char *name, const char *body)
{
    PP_Macro *m = pp_macro_find(name);
    if (m)
    {
        strncpy(m->body, body ? body : "", PP_MAX_MACRO_BODY - 1);
        m->body[PP_MAX_MACRO_BODY - 1] = '\0';
        m->is_function = 0;
        m->param_count = 0;
        return;
    }
    m = pp_macro_alloc();
    if (!m)
        die("macro pool exhausted");
    strncpy(m->name, name, PP_MAX_MACRO_NAME - 1);
    m->name[PP_MAX_MACRO_NAME - 1] = '\0';
    strncpy(m->body, body ? body : "", PP_MAX_MACRO_BODY - 1);
    m->body[PP_MAX_MACRO_BODY - 1] = '\0';
    m->is_function = 0;
    m->param_count = 0;
}

static void pp_macro_add_function(const char *name, char params[][PP_MAX_MACRO_PARAM_LEN], int pcount, const char *body)
{
    PP_Macro *m = pp_macro_find(name);
    if (m)
    {
        m->is_function = 1;
        m->param_count = (unsigned char)pcount;
        for (int i = 0; i < pcount && i < PP_MAX_MACRO_PARAMS; ++i)
        {
            strncpy(m->params[i], params[i], PP_MAX_MACRO_PARAM_LEN - 1);
            m->params[i][PP_MAX_MACRO_PARAM_LEN - 1] = '\0';
        }
        strncpy(m->body, body ? body : "", PP_MAX_MACRO_BODY - 1);
        m->body[PP_MAX_MACRO_BODY - 1] = '\0';
        return;
    }
    m = pp_macro_alloc();
    if (!m)
        die("macro pool exhausted");
    strncpy(m->name, name, PP_MAX_MACRO_NAME - 1);
    m->name[PP_MAX_MACRO_NAME - 1] = '\0';
    m->is_function = 1;
    m->param_count = (unsigned char)pcount;
    for (int i = 0; i < pcount && i < PP_MAX_MACRO_PARAMS; ++i)
    {
        strncpy(m->params[i], params[i], PP_MAX_MACRO_PARAM_LEN - 1);
        m->params[i][PP_MAX_MACRO_PARAM_LEN - 1] = '\0';
    }
    strncpy(m->body, body ? body : "", PP_MAX_MACRO_BODY - 1);
    m->body[PP_MAX_MACRO_BODY - 1] = '\0';
}

static int preclean_input_into(const unsigned char *in, unsigned char *out, size_t out_sz)
{
    size_t len = strlen((const char *)in);
    size_t wi = 0;
    for (size_t i = 0; i < len;)
    {
        if (in[i] == '/' && i + 1 < len && in[i + 1] == '/')
        {
            i += 2;
            while (i < len && in[i] != '\n')
                i++;
            continue;
        }
        if (in[i] == '/' && i + 1 < len && in[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < len && !(in[i] == '*' && in[i + 1] == '/'))
                i++;
            if (i + 1 < len)
                i += 2;
            continue;
        }
        if (in[i] == '\\' && i + 1 < len && in[i + 1] == '\n')
        {
            i += 2;
            continue;
        }
        if (wi + 1 >= out_sz)
            return -1;
        out[wi++] = in[i++];
    }
    if (wi >= out_sz)
        return -1;
    out[wi] = '\0';
    return (int)wi;
}

static long read_file_into(const char *path, unsigned char *dest, size_t dest_max)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return -1;
    }
    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return -1;
    }
    if ((size_t)sz >= dest_max)
    {
        sz = (long)(dest_max - 1);
        fseek(f, 0, SEEK_SET);
        size_t r = fread(dest, 1, (size_t)sz, f);
        dest[r] = '\0';
        fclose(f);
        return (long)r;
    }
    fseek(f, 0, SEEK_SET);
    size_t r = fread(dest, 1, (size_t)sz, f);
    dest[r] = '\0';
    fclose(f);
    return (long)r;
}

static int pp_is_ident_char(char c, int first)
{
    if (first)
        return (isalpha((unsigned char)c) || c == '_');
    return (isalnum((unsigned char)c) || c == '_');
}

static int parse_macro_args_into(const char *p, char args[PP_ARG_MAX][PP_ARG_LEN], int *out_argc, const char **p_after)
{
    const char *s = p;
    if (*s != '(')
        return -1;
    s++;
    int argc = 0;
    int level = 0;
    size_t cur = 0;
    while (*s)
    {
        if (*s == '(')
        {
            level++;
            if (cur < PP_ARG_LEN - 1)
                args[argc][cur++] = *s;
            s++;
            continue;
        }
        if (*s == ')' && level == 0)
        {
            if (argc < PP_ARG_MAX)
            {
                args[argc][cur] = '\0';
                argc++;
            }
            s++;
            break;
        }
        if (*s == ')')
        {
            level--;
            if (cur < PP_ARG_LEN - 1)
                args[argc][cur++] = *s;
            s++;
            continue;
        }
        if (*s == ',' && level == 0)
        {
            if (argc < PP_ARG_MAX)
            {
                args[argc][cur] = '\0';
                argc++;
                cur = 0;
            }
            s++;
            while (isspace((unsigned char)*s))
                s++;
            continue;
        }
        if (argc >= PP_ARG_MAX)
            return -1;
        if (cur + 1 < PP_ARG_LEN)
            args[argc][cur++] = *s;
        s++;
    }
    if (argc < PP_ARG_MAX)
    {
        args[argc][cur] = '\0';
    }
    if (p_after)
        *p_after = s;
    *out_argc = argc;
    return 0;
}

static int pp_macro_instantiate_to(PP_Macro *m, char args[PP_ARG_MAX][PP_ARG_LEN], int argc, char *outbuf, size_t outbuf_sz)
{
    if (!m)
        return -1;
    const char *body = m->body;
    size_t outi = 0;
    for (const char *s = body; *s;)
    {
        if (pp_is_ident_char(*s, 1))
        {
            const char *t = s + 1;
            while (*t && pp_is_ident_char(*t, 0))
                t++;
            size_t idlen = (size_t)(t - s);
            char ident[PP_MAX_MACRO_NAME];
            if (idlen >= sizeof(ident))
                idlen = sizeof(ident) - 1;
            memcpy(ident, s, idlen);
            ident[idlen] = '\0';
            int replaced = 0;
            for (int i = 0; i < m->param_count; ++i)
            {
                if (strcmp(ident, m->params[i]) == 0)
                {
                    const char *arg = (i < argc) ? args[i] : "";
                    size_t al = strlen(arg);
                    if (outi + al + 1 >= outbuf_sz)
                        return -1;
                    memcpy(outbuf + outi, arg, al);
                    outi += al;
                    replaced = 1;
                    break;
                }
            }
            if (!replaced)
            {
                if (outi + idlen + 1 >= outbuf_sz)
                    return -1;
                memcpy(outbuf + outi, s, idlen);
                outi += idlen;
            }
            s = t;
        }
        else
        {
            if (outi + 2 >= outbuf_sz)
                return -1;
            outbuf[outi++] = *s++;
        }
    }
    if (outi >= outbuf_sz)
        return -1;
    outbuf[outi] = '\0';
    return 0;
}

static int expand_macros_line_to(const char *line, char *outbuf, size_t outbuf_sz, int depth)
{
    if (depth > 64)
        return -1;
    size_t outi = 0;
    const char *p = line;
    while (*p)
    {
        if (pp_is_ident_char(*p, 1))
        {
            const char *t = p + 1;
            while (*t && pp_is_ident_char(*t, 0))
                t++;
            size_t idlen = (size_t)(t - p);
            char ident[PP_MAX_MACRO_NAME];
            if (idlen >= sizeof(ident))
                idlen = sizeof(ident) - 1;
            memcpy(ident, p, idlen);
            ident[idlen] = '\0';
            PP_Macro *m = pp_macro_find(ident);
            if (m)
            {
                if (m->is_function)
                {
                    const char *q = t;
                    while (*q && isspace((unsigned char)*q))
                        q++;
                    if (*q == '(')
                    {
                        char args[PP_ARG_MAX][PP_ARG_LEN];
                        int argc = 0;
                        const char *after;
                        if (parse_macro_args_into(q, args, &argc, &after) == 0)
                        {
                            static char temp_inst[PP_MAX_MACRO_BODY * 2];
                            if (pp_macro_instantiate_to(m, args, argc, temp_inst, sizeof(temp_inst)) != 0)
                                return -1;
                            static char temp_rec[PP_MAX_MACRO_BODY * 3];
                            if (expand_macros_line_to(temp_inst, temp_rec, sizeof(temp_rec), depth + 1) != 0)
                                return -1;
                            size_t rl = strlen(temp_rec);
                            if (outi + rl + 1 >= outbuf_sz)
                                return -1;
                            memcpy(outbuf + outi, temp_rec, rl);
                            outi += rl;
                            p = after;
                            continue;
                        }
                    }
                }
                else
                {
                    static char temp_body[PP_MAX_MACRO_BODY * 2];
                    if (expand_macros_line_to(m->body, temp_body, sizeof(temp_body), depth + 1) != 0)
                        return -1;
                    size_t rl = strlen(temp_body);
                    if (outi + rl + 1 >= outbuf_sz)
                        return -1;
                    memcpy(outbuf + outi, temp_body, rl);
                    outi += rl;
                    p = t;
                    continue;
                }
            }
            if (outi + idlen + 1 >= outbuf_sz)
                return -1;
            memcpy(outbuf + outi, p, idlen);
            outi += idlen;
            p = t;
        }
        else
        {
            if (outi + 2 >= outbuf_sz)
                return -1;
            outbuf[outi++] = *p++;
        }
    }
    if (outi >= outbuf_sz)
        return -1;
    outbuf[outi] = '\0';
    return 0;
}

static void trim_inplace(char *s)
{
    if (!s)
        return;
    char *p = s;
    while (isspace((unsigned char)*p))
        p++;
    if (p != s)
        memmove(s, p, strlen(p) + 1);
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1)))
        *(--end) = '\0';
}

static int eval_simple_if_expr_pp(const char *expr)
{
    while (isspace((unsigned char)*expr))
        expr++;
    if (strncmp(expr, "defined", 7) == 0)
    {
        const char *p = expr + 7;
        while (*p && isspace((unsigned char)*p))
            p++;
        char name[PP_MAX_MACRO_NAME];
        if (*p == '(')
        {
            p++;
            const char *e = strchr(p, ')');
            if (!e)
                return 0;
            size_t n = (size_t)(e - p);
            if (n >= sizeof(name))
                n = sizeof(name) - 1;
            memcpy(name, p, n);
            name[n] = '\0';
            trim_inplace(name);
            return pp_macro_find(name) ? 1 : 0;
        }
        else
        {
            const char *q = p;
            int i = 0;
            while (*q && (isalnum((unsigned char)*q) || *q == '_') && i + 1 < (int)sizeof(name))
                name[i++] = *q++;
            name[i] = '\0';
            trim_inplace(name);
            if (strlen(name) == 0)
                return 0;
            return pp_macro_find(name) ? 1 : 0;
        }
    }
    else if (*expr == '!')
    {
        return !eval_simple_if_expr_pp(expr + 1);
    }
    else
    {
        while (isspace((unsigned char)*expr))
            expr++;
        if (*expr == '0')
            return 0;
        if (*expr == '1')
            return 1;
        return 0;
    }
}

static const char *pp_find_include_file(const char *name, char *out_path, size_t out_sz)
{
    const char *search_paths[] = {"/lib/include/", "/lib/", "/include/", "./", NULL};
    if (name[0] == '/' || (name[0] == '.' && name[1] == '/') || (name[0] == '.' && name[1] == '.'))
    {
        FILE *f = fopen(name, "rb");
        if (f)
        {
            fclose(f);
            strncpy(out_path, name, out_sz - 1);
            out_path[out_sz - 1] = '\0';
            return out_path;
        }
    }
    for (int i = 0; search_paths[i]; ++i)
    {
        size_t need = strlen(search_paths[i]) + strlen(name) + 2;
        if (need > out_sz)
            continue;
        snprintf(out_path, out_sz, "%s%s", search_paths[i], name);
        FILE *f = fopen(out_path, "rb");
        if (f)
        {
            fclose(f);
            return out_path;
        }
    }
    return NULL;
}

static void preprocess_src(const unsigned char *in, unsigned char *out, size_t out_max)
{
    if (!in || !out)
        die("preprocess_src: bad args (no heap allowed)");
    pp_macros_clear_all();

    static unsigned char clean_buf[PP_FILE_BUF_SIZE];
    if (preclean_input_into(in, clean_buf, sizeof(clean_buf)) < 0)
        die("preclean_input failed");

    static unsigned char inc_buf[PP_MAX_DEPTH][PP_FILE_BUF_SIZE];
    static unsigned char inc_out[PP_MAX_DEPTH][PP_OUT_MAX];
    static char include_stack[PP_MAX_DEPTH][512];
    /* per-depth current scan pointer (char pointer into inc_buf[depth]) */
    const char *cur_pos[PP_MAX_DEPTH];

    /* copy cleaned input into depth 0 */
    strncpy((char *)inc_buf[0], (const char *)clean_buf, PP_FILE_BUF_SIZE - 1);
    inc_buf[0][PP_FILE_BUF_SIZE - 1] = '\0';
    include_stack[0][0] = '\0';
    cur_pos[0] = (const char *)inc_buf[0];

    int current_depth = 0;
    size_t out_len = 0;

    int cond_stack[PP_MAX_DEPTH];
    int cond_top = 0;
    cond_stack[0] = 1;
    cond_top = 1;

    while (current_depth >= 0)
    {
        /* current buffer and position */
        unsigned char *curbuf = inc_buf[current_depth];
        const char *p = cur_pos[current_depth]; /* start from saved pos */

        char linebuf[PP_LINEBUF];
        while (*p)
        {
            const char *line_start = p;
            const char *nl = strchr(p, '\n');
            size_t line_len = nl ? (size_t)(nl - p) : strlen(p);
            if (line_len >= sizeof(linebuf))
                die("line too long in preprocess_src");
            memcpy(linebuf, line_start, line_len);
            linebuf[line_len] = '\0';
            /* advance p to next line and store back to cur_pos */
            p = nl ? (nl + 1) : (p + line_len);
            cur_pos[current_depth] = p;

            const char *s = linebuf;
            while (*s && isspace((unsigned char)*s))
                s++;

            if (*s == '#')
            {
                s++;
                while (*s && isspace((unsigned char)*s))
                    s++;

                if (strncmp(s, "include", 7) == 0 && isspace((unsigned char)s[7]))
                {
                    s += 7;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    if (*s == '"' || *s == '<')
                    {
                        char endc = (*s == '"') ? '"' : '>';
                        s++;
                        const char *e = strchr(s, endc);
                        if (!e)
                        {
                            fprintf(stderr, "preprocessor error: unterminated include directive\n");
                            die("include parse failed");
                        }
                        size_t n = (size_t)(e - s);
                        if (n >= 511)
                            n = 511;
                        char incname[512];
                        memcpy(incname, s, n);
                        incname[n] = '\0';
                        char pathbuf[512];
                        const char *found = pp_find_include_file(incname, pathbuf, sizeof(pathbuf));
                        if (!found)
                        {
                            const char *caller = include_stack[current_depth][0] ? include_stack[current_depth] : "<input>";
                            fprintf(stderr, "preprocessor error: include file not found: \"%s\" (included from %s)\n", incname, caller);
                            die("include not found");
                        }
                        if (current_depth + 1 >= PP_MAX_DEPTH)
                            die("include recursion too deep");
                        int cyc = 0;
                        for (int i = 0; i <= current_depth; ++i)
                        {
                            if (strcmp(include_stack[i], pathbuf) == 0)
                            {
                                cyc = 1;
                                break;
                            }
                        }
                        if (cyc)
                        {
                            /* skip cyclic include silently (avoid infinite recursion) */
                            continue;
                        }
                        long r = read_file_into(pathbuf, inc_buf[current_depth + 1], PP_FILE_BUF_SIZE);
                        if (r < 0)
                        {
                            fprintf(stderr, "preprocessor error: failed to read include file: \"%s\" (from %s)\n",
                                    pathbuf,
                                    include_stack[current_depth][0] ? include_stack[current_depth] : "<input>");
                            die("include read failed");
                        }
                        unsigned char tmp_clean[PP_FILE_BUF_SIZE];
                        if (preclean_input_into(inc_buf[current_depth + 1], tmp_clean, sizeof(tmp_clean)) < 0)
                            die("preclean_input failed");
                        strncpy((char *)inc_buf[current_depth + 1], (const char *)tmp_clean, PP_FILE_BUF_SIZE - 1);
                        inc_buf[current_depth + 1][PP_FILE_BUF_SIZE - 1] = '\0';
                        strncpy(include_stack[current_depth + 1], pathbuf, sizeof(include_stack[0]) - 1);
                        include_stack[current_depth + 1][sizeof(include_stack[0]) - 1] = '\0';
                        /* push: initialize new depth's cursor to its buffer start and switch to it */
                        current_depth++;
                        cur_pos[current_depth] = (const char *)inc_buf[current_depth];
                        /* update local p for next inner iteration to use new depth */
                        p = cur_pos[current_depth];
                        /* continue processing inside new buffer immediately */
                        continue;
                    }
                }
                else if (strncmp(s, "define", 6) == 0 && isspace((unsigned char)s[6]))
                {
                    s += 6;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    char name[PP_MAX_MACRO_NAME];
                    int ni = 0;
                    if (!pp_is_ident_char(*s, 1))
                        continue;
                    name[ni++] = *s++;
                    while (*s && pp_is_ident_char(*s, 0) && ni + 1 < (int)sizeof(name))
                        name[ni++] = *s++;
                    name[ni] = '\0';

                    /* function-like only if '(' immediately follows name */
                    const char *after_name = s;
                    if (*after_name == '(')
                    {
                        s = after_name + 1;
                        char params[PP_MAX_MACRO_PARAMS][PP_MAX_MACRO_PARAM_LEN];
                        int pcount = 0;
                        while (*s && *s != ')' && pcount < PP_MAX_MACRO_PARAMS)
                        {
                            while (*s && isspace((unsigned char)*s))
                                s++;
                            char pname[PP_MAX_MACRO_PARAM_LEN];
                            int pi = 0;
                            if (!pp_is_ident_char(*s, 1))
                                break;
                            pname[pi++] = *s++;
                            while (*s && pp_is_ident_char(*s, 0) && pi + 1 < (int)sizeof(pname))
                                pname[pi++] = *s++;
                            pname[pi] = '\0';
                            strncpy(params[pcount], pname, PP_MAX_MACRO_PARAM_LEN - 1);
                            params[pcount][PP_MAX_MACRO_PARAM_LEN - 1] = '\0';
                            pcount++;
                            while (*s && isspace((unsigned char)*s))
                                s++;
                            if (*s == ',')
                            {
                                s++;
                                continue;
                            }
                        }
                        if (*s == ')')
                            s++;
                        while (*s && isspace((unsigned char)*s))
                            s++;
                        char body[PP_MAX_MACRO_BODY];
                        strncpy(body, s, sizeof(body) - 1);
                        body[sizeof(body) - 1] = '\0';
                        trim_inplace(body);
                        pp_macro_add_function(name, params, pcount, body);
                        continue;
                    }
                    else
                    {
                        while (*s && isspace((unsigned char)*s))
                            s++;
                        char body[PP_MAX_MACRO_BODY];
                        strncpy(body, s, sizeof(body) - 1);
                        body[sizeof(body) - 1] = '\0';
                        trim_inplace(body);
                        pp_macro_add_object(name, body);
                        continue;
                    }
                }
                else if (strncmp(s, "undef", 5) == 0 && isspace((unsigned char)s[5]))
                {
                    s += 5;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    char name[PP_MAX_MACRO_NAME];
                    int ni = 0;
                    while (*s && pp_is_ident_char(*s, (ni == 0)))
                    {
                        if (ni + 1 < (int)sizeof(name))
                            name[ni++] = *s;
                        s++;
                    }
                    name[ni] = '\0';
                    trim_inplace(name);
                    if (strlen(name))
                        pp_macro_undef(name);
                    continue;
                }
                else if (strncmp(s, "ifdef", 5) == 0 && isspace((unsigned char)s[5]))
                {
                    s += 5;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    char name[PP_MAX_MACRO_NAME];
                    int ni = 0;
                    while (*s && pp_is_ident_char(*s, (ni == 0)))
                    {
                        if (ni + 1 < (int)sizeof(name))
                            name[ni++] = *s;
                        s++;
                    }
                    name[ni] = '\0';
                    trim_inplace(name);
                    int val = pp_macro_find(name) ? 1 : 0;
                    if (cond_top >= PP_MAX_DEPTH)
                        die("conditional depth");
                    cond_stack[cond_top] = (cond_stack[cond_top - 1] && val) ? 1 : 0;
                    cond_top++;
                    continue;
                }
                else if (strncmp(s, "ifndef", 6) == 0 && isspace((unsigned char)s[6]))
                {
                    s += 6;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    char name[PP_MAX_MACRO_NAME];
                    int ni = 0;
                    while (*s && pp_is_ident_char(*s, (ni == 0)))
                    {
                        if (ni + 1 < (int)sizeof(name))
                            name[ni++] = *s;
                        s++;
                    }
                    name[ni] = '\0';
                    trim_inplace(name);
                    int val = pp_macro_find(name) ? 0 : 1;
                    if (cond_top >= PP_MAX_DEPTH)
                        die("conditional depth");
                    cond_stack[cond_top] = (cond_stack[cond_top - 1] && val) ? 1 : 0;
                    cond_top++;
                    continue;
                }
                else if (strncmp(s, "if", 2) == 0 && isspace((unsigned char)s[2]))
                {
                    s += 2;
                    while (*s && isspace((unsigned char)*s))
                        s++;
                    int val = eval_simple_if_expr_pp(s);
                    if (cond_top >= PP_MAX_DEPTH)
                        die("conditional depth");
                    cond_stack[cond_top] = (cond_stack[cond_top - 1] && val) ? 1 : 0;
                    cond_top++;
                    continue;
                }
                else if (strncmp(s, "elif", 4) == 0 && isspace((unsigned char)s[4]))
                {
                    if (cond_top <= 1)
                        continue;
                    if (cond_stack[cond_top - 1])
                        cond_stack[cond_top - 1] = 0;
                    else
                    {
                        int val = eval_simple_if_expr_pp(s + 4);
                        cond_stack[cond_top - 1] = cond_stack[cond_top - 2] ? (val ? 1 : 0) : 0;
                    }
                    continue;
                }
                else if (strncmp(s, "else", 4) == 0 && (s[4] == '\0' || isspace((unsigned char)s[4])))
                {
                    if (cond_top <= 1)
                        continue;
                    if (cond_stack[cond_top - 2])
                        cond_stack[cond_top - 1] = !cond_stack[cond_top - 1];
                    else
                        cond_stack[cond_top - 1] = 0;
                    continue;
                }
                else if (strncmp(s, "endif", 5) == 0 && (s[5] == '\0' || isspace((unsigned char)s[5])))
                {
                    if (cond_top <= 1)
                        continue;
                    cond_top--;
                    continue;
                }
                else
                {
                    continue;
                }
            }

            if (!cond_stack[cond_top - 1])
                continue;

            char expanded[PP_OUT_MAX];
            if (expand_macros_line_to(linebuf, expanded, sizeof(expanded), 0) != 0)
                die("macro expansion error");
            size_t elen = strlen(expanded);
            if (out_len + elen + 2 > out_max)
                die("preprocessed source too large");
            memcpy(out + out_len, expanded, elen);
            out_len += elen;
            out[out_len++] = '\n';
            out[out_len] = '\0';
        } /* end while lines in current depth */

        /* finished current buffer: pop or finish whole preprocess */
        if (current_depth == 0)
            break;
        /* clear include_stack entry for popped depth (optional) */
        include_stack[current_depth][0] = '\0';
        current_depth--;
        /* no need to reset cur_pos[current_depth] because it already contains parent's next pos */
    } /* end while depth stack */

    if (out_len >= out_max)
        die("preprocess output overflow");
    out[out_len] = '\0';
}

static int struct_find(const char *name)
{
    if (!name)
        return -1;
    for (int i = 0; i < structsyms_cnt; ++i)
        if (strcmp(structsyms[i].name, name) == 0)
            return i;
    return -1;
}

static int add_structsym(const char *name, int is_union)
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

static int add_struct_field(int si, const char *fname, int field_size)
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

static int struct_field_offset(int si, const char *fname)
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

static int struct_field_size(int si, const char *fname)
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

static int is_typedef_name(const char *name)
{
    for (int i = 0; i < typedefs_cnt; ++i)
        if (strcmp(typedefs[i].name, name) == 0)
            return 1;
    return 0;
}

static void add_typedef_name(const char *name)
{
    if (typedefs_cnt >= MAX_TYPEDEFS)
        die("too many typedefs");

    strncpy(typedefs[typedefs_cnt].name, name, sizeof(typedefs[typedefs_cnt].name) - 1);
    typedefs[typedefs_cnt].name[sizeof(typedefs[typedefs_cnt].name) - 1] = '\0';
    typedefs_cnt++;
}

static int enum_find(const char *name)
{
    for (int i = 0; i < enum_consts_cnt; ++i)
        if (strcmp(enum_consts[i].name, name) == 0)
            return i;
    return -1;
}

static void add_enum_const(const char *name, int32_t val)
{
    if (enum_consts_cnt >= MAX_ENUM_CONSTS)
        die("too many enum constants");

    strncpy(enum_consts[enum_consts_cnt].name, name, sizeof(enum_consts[enum_consts_cnt].name) - 1);
    enum_consts[enum_consts_cnt].name[sizeof(enum_consts[enum_consts_cnt].name) - 1] = '\0';
    enum_consts[enum_consts_cnt].val = val;
    enum_consts_cnt++;
}

static void add_string_literal_entry(const char *s)
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

static int is_type_name(const char *name)
{
    if (!name)
        return 0;
    if (strcmp(name, "int") == 0)
        return 1;
    if (strcmp(name, "void") == 0)
        return 1;
    if (strcmp(name, "unsigned") == 0)
        return 1;
    if (strcmp(name, "signed") == 0)
        return 1;
    if (strcmp(name, "char") == 0)
        return 1;
    if (strcmp(name, "short") == 0)
        return 1;
    if (strcmp(name, "long") == 0)
        return 1;
    if (strcmp(name, "struct") == 0)
        return 1;
    if (strcmp(name, "union") == 0)
        return 1;
    return is_typedef_name(name);
}

char *skip_ws(char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

void next_token()
{
    curtok.type = TOK_EOF;
    curtok.ident[0] = 0;
    curtok.val = 0;
    curtok.sym = 0;
    curtok.op = 0;

    while (isspace((unsigned char)*src))
        src++;

    if (!*src)
        return;

    if (isalpha((unsigned char)*src) || *src == '_')
    {
        char *p = curtok.ident;
        while (isalnum((unsigned char)*src) || *src == '_')
            *p++ = *src++;
        *p = 0;
        curtok.type = TOK_IDENT;
        return;
    }

    if (isdigit((unsigned char)*src) || (*src == '-' && isdigit((unsigned char)*(src + 1))))
    {
        char *end;
        curtok.val = (int32_t)strtol(src, &end, 10);
        curtok.type = TOK_NUMBER;
        src = end;
        return;
    }

    if (src[0] == '&' && src[1] == '&')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LAND;
        src += 2;
        return;
    }
    if (src[0] == '|' && src[1] == '|')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LOR;
        src += 2;
        return;
    }

    if ((src[0] == '=' && src[1] == '=') ||
        (src[0] == '!' && src[1] == '=') ||
        (src[0] == '<' && src[1] == '=') ||
        (src[0] == '>' && src[1] == '='))
    {
        curtok.type = TOK_OP;
        if (src[0] == '=' && src[1] == '=')
            curtok.op = OP_EQ;
        else if (src[0] == '!' && src[1] == '=')
            curtok.op = OP_NE;
        else if (src[0] == '<' && src[1] == '=')
            curtok.op = OP_LE;
        else
            curtok.op = OP_GE;
        src += 2;
        return;
    }

    if (*src == '!')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LNOT;
        src++;
        return;
    }
    if (*src == '&')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BAND;
        src++;
        return;
    }
    if (*src == '|')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BOR;
        src++;
        return;
    }
    if (*src == '^')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BXOR;
        src++;
        return;
    }

    if (src[0] == '<' && src[1] == '<')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_SHL;
        src += 2;
        return;
    }
    if (src[0] == '>' && src[1] == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_SHR;
        src += 2;
        return;
    }

    if (src[0] == '+' && src[1] == '+')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_INC;
        src += 2;
        return;
    }
    if (src[0] == '-' && src[1] == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_ARROW;
        src += 2;
        return;
    }
    if (src[0] == '-' && src[1] == '-')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_DEC;
        src += 2;
        return;
    }

    if (*src == '<')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LT;
        src++;
        return;
    }
    if (*src == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_GT;
        src++;
        return;
    }

    if (*src == '"')
    {
        src++;
        char *p = curtok.str;
        while (*src && *src != '"')
        {
            if (*src == '\\')
            {
                src++;
                if (!*src)
                    break;
                switch (*src)
                {
                case 'n':
                    *p++ = '\n';
                    break;
                case 't':
                    *p++ = '\t';
                    break;
                case '\\':
                    *p++ = '\\';
                    break;
                case '"':
                    *p++ = '"';
                    break;
                case '\'':
                    *p++ = '\'';
                    break;
                case '0':
                    die("\\0 within string literal not supported");
                    break;
                default:
                    *p++ = *src;
                    break;
                }
                src++;
            }
            else
            {
                *p++ = *src++;
            }
        }
        *p = '\0';
        if (*src != '"')
            die("unterminated string literal");
        src++;
        curtok.type = TOK_STRING;
        return;
    }

    if (*src == '\'')
    {
        src++;
        int v = 0;
        if (*src == '\\')
        {
            src++;
            if (!*src)
                die("unterminated char literal");
            switch (*src)
            {
            case 'n':
                v = '\n';
                break;
            case 't':
                v = '\t';
                break;
            case '\\':
                v = '\\';
                break;
            case '\'':
                v = '\'';
                break;
            case '0':
                v = '\0';
                break;
            default:
                v = (unsigned char)*src;
                break;
            }
            src++;
        }
        else
        {
            if (!*src)
                die("unterminated char literal");
            v = (unsigned char)*src;
            src++;
        }
        if (*src != '\'')
            die("unterminated char literal");
        src++;
        curtok.type = TOK_NUMBER;
        curtok.val = v;
        return;
    }

    curtok.type = TOK_SYM;
    curtok.sym = *src++;
}

int accept_ident(const char *s)
{
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, s) == 0)
    {
        next_token();
        return 1;
    }
    return 0;
}

int accept_sym(char c)
{
    if (curtok.type == TOK_SYM && curtok.sym == c)
    {
        next_token();
        return 1;
    }
    return 0;
}

void expect_sym(char c)
{
    if (!accept_sym(c))
        die("expected symbol");
}

void expect_ident()
{
    if (curtok.type != TOK_IDENT)
        die("expected identifier");
}

void emit_u8(unsigned char b)
{
    if (code_len + 1 > MAX_CODE)
        die("code overflow");

    code_buf[code_len++] = b;
}
void emit_u16(uint16_t v)
{
    emit_u8(v & 0xFF);
    emit_u8((v >> 8) & 0xFF);
}
void emit_u32(uint32_t v)
{
    emit_u8(v & 0xFF);
    emit_u8((v >> 8) & 0xFF);
    emit_u8((v >> 16) & 0xFF);
    emit_u8((v >> 24) & 0xFF);
}

void emit_push_imm32(uint32_t v)
{
    emit_u8(0x68);
    emit_u32(v);
}
void emit_pop_eax() { emit_u8(0x58); }
void emit_pop_ebx() { emit_u8(0x5B); }
void emit_push_eax() { emit_u8(0x50); }
void emit_mov_eax_from_ebp_disp_any(int32_t disp)
{
    if (disp >= -128 && disp <= 127)
    {
        emit_u8(0x8B);
        emit_u8(0x45);
        emit_u8((uint8_t)disp);
    }
    else
    {
        emit_u8(0x8B);
        emit_u8(0x85);
        emit_u32((uint32_t)disp);
    }
}
void emit_mov_ebp_disp_from_eax_any(int32_t disp)
{
    if (disp >= -128 && disp <= 127)
    {
        emit_u8(0x89);
        emit_u8(0x45);
        emit_u8((uint8_t)disp);
    }
    else
    {
        emit_u8(0x89);
        emit_u8(0x85);
        emit_u32((uint32_t)disp);
    }
}
void emit_lea_eax_ebp_disp_any(int32_t disp)
{
    if (disp >= -128 && disp <= 127)
    {
        emit_u8(0x8D);
        emit_u8(0x45);
        emit_u8((uint8_t)disp);
    }
    else
    {
        emit_u8(0x8D);
        emit_u8(0x85);
        emit_u32((uint32_t)disp);
    }
}

void emit_load_eax_from_eax_ptr()
{
    emit_u8(0x8B);
    emit_u8(0x00);
}

void emit_mov_ptr_ebx_from_eax()
{
    emit_u8(0x89);
    emit_u8(0x03);
}

void emit_shl_ebx_imm(uint8_t imm)
{
    emit_u8(0xC1);
    emit_u8(0xE3);
    emit_u8(imm);
}

void emit_add_eax_ebx()
{
    emit_u8(0x01);
    emit_u8(0xD8);
}
void emit_sub_eax_ebx()
{
    emit_u8(0x29);
    emit_u8(0xD8);
}
void emit_imul_eax_ebx()
{
    emit_u8(0x0F);
    emit_u8(0xAF);
    emit_u8(0xC3);
}
void emit_cdq() { emit_u8(0x99); }
void emit_idiv_ebx()
{
    emit_u8(0xF7);
    emit_u8(0xFB);
}
void emit_test_eax_eax()
{
    emit_u8(0x85);
    emit_u8(0xC0);
}
size_t emit_jz_rel32_placeholder()
{
    emit_u8(0x0F);
    emit_u8(0x84);
    size_t off = code_len;
    emit_u32(0);
    return off;
}
size_t emit_jnz_rel32_placeholder()
{
    emit_u8(0x0F);
    emit_u8(0x85);
    size_t off = code_len;
    emit_u32(0);
    return off;
}
size_t emit_jmp_rel32_placeholder()
{
    emit_u8(0xE9);
    size_t off = code_len;
    emit_u32(0);
    return off;
}
void patch_rel32_at(size_t place, uint32_t rel32)
{
    if (place + 4 > code_len)
        die("internal: patch out of bounds");
    memcpy(code_buf + place, &rel32, 4);
}
size_t emit_call_rel32_placeholder()
{
    emit_u8(0xE8);
    size_t off = code_len;
    emit_u32(0);
    return off;
}
void emit_leave() { emit_u8(0xC9); }
void emit_ret_pop_args_uint16(uint16_t argbytes)
{
    emit_u8(0xC2);
    emit_u16(argbytes);
}
void emit_sub_esp_imm(uint32_t imm)
{
    if (imm == 0)
        return;
    if (imm <= 0x7F)
    {
        emit_u8(0x83);
        emit_u8(0xEC);
        emit_u8((uint8_t)imm);
    }
    else
    {
        emit_u8(0x81);
        emit_u8(0xEC);
        emit_u32(imm);
    }
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
    relocs[relocs_cnt].call_site = call_site;
    relocs_cnt++;
}

int local_find(const char *name)
{
    for (int i = 0; i < locals_cnt; i++)
        if (strcmp(locals[i].name, name) == 0)
            return i;
    return -1;
}

int alloc_local(const char *name, int is_const)
{
    if (locals_cnt >= MAX_LOCALS)
        die("too many locals");

    locals[locals_cnt].offset = local_stack_size + 4;
    strncpy(locals[locals_cnt].name, name, sizeof(locals[locals_cnt].name) - 1);
    locals[locals_cnt].name[sizeof(locals[locals_cnt].name) - 1] = '\0';
    locals[locals_cnt].is_const = is_const ? 1 : 0;
    locals[locals_cnt].ptr_depth = 0;
    locals[locals_cnt].arr_size = 0;
    locals[locals_cnt].struct_id = -1;
    locals_cnt++;
    local_stack_size += 4;
    return locals_cnt - 1;
}

static inline int32_t neg_off(int offset) { return -(int32_t)offset; }

void gen_expr();
void gen_cmp();
void gen_ternary();

void gen_assign()
{
    if (curtok.type == TOK_IDENT)
    {
        Token save = curtok;
        char *save_src = src;

        next_token();

        if (curtok.type == TOK_SYM && curtok.sym == '=')
        {
            next_token();
            gen_expr();
            int li = local_find(save.ident);
            if (li < 0)
                die("unknown variable on assign");
            if (locals[li].is_const)
                die("assignment to const variable");
            emit_pop_eax();
            emit_mov_ebp_disp_from_eax_any(neg_off(locals[li].offset));
            emit_push_eax();
            return;
        }

        if (curtok.type == TOK_SYM && curtok.sym == '[')
        {
            int li = local_find(save.ident);
            if (li < 0)
                die("unknown variable on assign");
            if (locals[li].is_const)
                die("assignment to const variable");
            int offset = locals[li].offset;
            if (locals[li].arr_size > 0)
                emit_lea_eax_ebp_disp_any(neg_off(offset));
            else
                emit_mov_eax_from_ebp_disp_any(neg_off(offset));
            emit_push_eax();

            while (curtok.type == TOK_SYM && curtok.sym == '[')
            {
                next_token();
                gen_expr();
                expect_sym(']');
                emit_pop_ebx();
                emit_pop_eax();
                emit_shl_ebx_imm(2);
                emit_add_eax_ebx();
                emit_push_eax();
            }

            if (curtok.type == TOK_SYM && curtok.sym == '=')
            {
                next_token();
                gen_expr();
                emit_pop_eax();
                emit_pop_ebx();
                emit_mov_ptr_ebx_from_eax();
                emit_push_eax();
                return;
            }

            curtok = save;
            src = save_src;
        }

        if (curtok.type == TOK_SYM && (curtok.sym == '+' || curtok.sym == '-' ||
                                       curtok.sym == '*' || curtok.sym == '/' || curtok.sym == '%'))
        {
            char opch = curtok.sym;
            if (*src == '=')
            {
                src++;
                int li = local_find(save.ident);
                if (li < 0)
                    die("unknown variable on compound assign");
                if (locals[li].is_const)
                    die("assignment to const variable");
                int offset = locals[li].offset;
                emit_mov_eax_from_ebp_disp_any(neg_off(offset));
                emit_push_eax();
                next_token();
                gen_expr();
                emit_pop_ebx();
                emit_pop_eax();
                switch (opch)
                {
                case '+':
                    emit_add_eax_ebx();
                    break;
                case '-':
                    emit_sub_eax_ebx();
                    break;
                case '*':
                    emit_imul_eax_ebx();
                    break;
                case '/':
                    emit_cdq();
                    emit_idiv_ebx();
                    break;
                case '%':
                    emit_cdq();
                    emit_idiv_ebx();
                    emit_u8(0x89);
                    emit_u8(0xD0);
                    break;
                }
                emit_mov_ebp_disp_from_eax_any(neg_off(offset));
                emit_push_eax();
                return;
            }
        }

        if ((curtok.type == TOK_SYM && curtok.sym == '.') ||
            (curtok.type == TOK_OP && curtok.op == OP_ARROW))
        {
            int li = local_find(save.ident);
            if (li < 0)
                die("unknown variable on assign");
            if (locals[li].is_const)
                die("assignment to const variable");
            int base_offset = locals[li].offset;

            if (curtok.type == TOK_OP && curtok.op == OP_ARROW)
                emit_mov_eax_from_ebp_disp_any(neg_off(base_offset));
            else
                emit_lea_eax_ebp_disp_any(neg_off(base_offset));
            emit_push_eax();

            while ((curtok.type == TOK_SYM && curtok.sym == '.') ||
                   (curtok.type == TOK_OP && curtok.op == OP_ARROW))
            {
                int is_arrow = (curtok.type == TOK_OP && curtok.op == OP_ARROW);
                next_token();
                expect_ident();
                char fname[64];
                strncpy(fname, curtok.ident, 63);
                fname[63] = '\0';
                next_token();

                int si = locals[li].struct_id;
                if (si < 0)
                    die("not a struct/union");
                int off = struct_field_offset(si, fname);
                if (off < 0)
                    die("no such field");

                emit_pop_eax();
                if (off != 0)
                {
                    emit_u8(0x05);
                    emit_u32((uint32_t)off);
                }
                emit_push_eax();

                if (is_arrow)
                {
                    emit_load_eax_from_eax_ptr();
                    emit_push_eax();
                }
            }

            if (curtok.type == TOK_SYM && curtok.sym == '=')
            {
                next_token();
                gen_expr();
                emit_pop_eax();
                emit_pop_ebx();
                emit_mov_ptr_ebx_from_eax();
                emit_push_eax();
                return;
            }

            curtok = save;
            src = save_src;
        }

        curtok = save;
        src = save_src;
    }

    gen_ternary();
}

void emit_setcc_and_push(uint8_t setcc_op)
{
    emit_u8(0x0F);
    emit_u8(setcc_op);
    emit_u8(0xC0);
    emit_u8(0x0F);
    emit_u8(0xB6);
    emit_u8(0xC0);
    emit_push_eax();
}

void gen_primary()
{
    if (curtok.type == TOK_NUMBER)
    {
        emit_push_imm32((uint32_t)curtok.val);
        next_token();
        last_primary_is_simple = 0;
        return;
    }
    if (curtok.type == TOK_STRING)
    {
        emit_push_imm32(0);
        add_string_literal_entry(curtok.str);
        next_token();
        last_primary_is_simple = 0;
        return;
    }
    if (curtok.type == TOK_IDENT)
    {
        char name[64];
        strncpy(name, curtok.ident, 63);
        name[63] = '\0';
        next_token();

        if (strcmp(name, "sizeof") == 0)
        {
            if (!accept_sym('('))
                die("expected '(' after sizeof");

            if (curtok.type == TOK_IDENT && is_type_name(curtok.ident))
            {
                next_token();
                expect_sym(')');
                emit_push_imm32((uint32_t)4);
                last_primary_is_simple = 0;
                return;
            }
            else
            {
                gen_expr();
                expect_sym(')');
                emit_pop_eax();
                emit_push_imm32((uint32_t)4);
                last_primary_is_simple = 0;
                return;
            }
        }

        if (accept_sym('('))
        {
            int argc = 0;
            if (!accept_sym(')'))
            {
                gen_expr();
                argc++;
                while (accept_sym(','))
                {
                    gen_expr();
                    argc++;
                }
                expect_sym(')');
            }
            size_t call_site = emit_call_rel32_placeholder();
            add_reloc(name, (uint32_t)call_site);
            emit_push_eax();
            last_primary_is_simple = 0;
            return;
        }
        else
        {
            int li = local_find(name);
            if (li < 0)
            {
                int ei = enum_find(name);
                if (ei >= 0)
                {
                    emit_push_imm32((uint32_t)enum_consts[ei].val);
                    last_primary_is_simple = 0;
                    return;
                }
                die("unknown variable");
            }

            int offset = locals[li].offset;
            int had_index = 0;

            if (locals[li].arr_size > 0)
            {
                emit_lea_eax_ebp_disp_any(neg_off(offset));
                emit_push_eax();
            }
            else
            {
                emit_mov_eax_from_ebp_disp_any(neg_off(offset));
                emit_push_eax();
            }

            last_primary_is_simple = 1;
            strncpy(last_primary_ident, name, sizeof(last_primary_ident) - 1);
            last_primary_ident[sizeof(last_primary_ident) - 1] = '\0';

            while (curtok.type == TOK_SYM && curtok.sym == '[')
            {
                had_index = 1;
                last_primary_is_simple = 0;
                next_token();
                gen_expr();
                expect_sym(']');

                emit_pop_ebx();
                emit_pop_eax();

                emit_shl_ebx_imm(2);

                emit_add_eax_ebx();

                emit_load_eax_from_eax_ptr();
                emit_push_eax();
            }

            while ((curtok.type == TOK_SYM && curtok.sym == '.') ||
                   (curtok.type == TOK_OP && curtok.op == OP_ARROW))
            {
                int use_ptr = (curtok.type == TOK_OP && curtok.op == OP_ARROW);
                next_token();
                expect_ident();
                char fname[64];
                strncpy(fname, curtok.ident, 63);
                fname[63] = '\0';
                next_token();

                int si = locals[li].struct_id;
                if (si < 0)
                    die("not a struct/union");
                int off = struct_field_offset(si, fname);
                if (off < 0)
                    die("no such field");

                emit_pop_eax();
                if (!use_ptr)
                {
                    emit_lea_eax_ebp_disp_any(neg_off(offset));
                }
                if (off != 0)
                {
                    emit_u8(0x05);
                    emit_u32(off);
                }
                emit_load_eax_from_eax_ptr();
                emit_push_eax();
            }

            if (!had_index && curtok.type == TOK_OP && (curtok.op == OP_INC || curtok.op == OP_DEC))
            {
                int op = curtok.op;
                if (locals[li].is_const)
                    die("increment/decrement of const variable");
                if (locals[li].arr_size > 0)
                    die("increment/decrement of array");

                emit_pop_eax();
                emit_push_eax();

                if (op == OP_INC)
                {
                    int inc = (locals[li].ptr_depth > 0) ? 4 : 1;
                    emit_u8(0x83);
                    emit_u8(0xC0);
                    emit_u8((uint8_t)inc);
                }
                else
                {
                    int dec = (locals[li].ptr_depth > 0) ? 4 : 1;
                    emit_u8(0x83);
                    emit_u8(0xE8);
                    emit_u8((uint8_t)dec);
                }

                emit_mov_ebp_disp_from_eax_any(neg_off(offset));
                next_token();
                last_primary_is_simple = 0;
            }

            return;
        }
    }
    if (curtok.type == TOK_SYM && curtok.sym == '(')
    {
        next_token();
        gen_expr();
        expect_sym(')');
        last_primary_is_simple = 0;
        return;
    }
    die("unexpected primary");
}

void gen_unary()
{
    if (curtok.type == TOK_OP && curtok.op == OP_LNOT)
    {
        next_token();
        gen_unary();
        emit_pop_eax();
        emit_test_eax_eax();
        emit_setcc_and_push(0x94);
        return;
    }

    if (curtok.type == TOK_SYM && curtok.sym == '-')
    {
        next_token();
        gen_unary();
        emit_pop_eax();
        emit_u8(0xF7);
        emit_u8(0xD8);
        emit_push_eax();
        return;
    }

    if (curtok.type == TOK_OP && curtok.op == OP_BAND)
    {
        next_token();
        if (curtok.type == TOK_IDENT)
        {
            int li = local_find(curtok.ident);
            if (li < 0)
                die("address-of: unknown variable");
            emit_lea_eax_ebp_disp_any(neg_off(locals[li].offset));
            emit_push_eax();
            next_token();
            return;
        }
        else
        {
            die("address-of: unsupported expression");
        }
    }

    if (curtok.type == TOK_SYM && curtok.sym == '*')
    {
        next_token();
        gen_unary();
        emit_pop_eax();
        emit_load_eax_from_eax_ptr();
        emit_push_eax();
        return;
    }

    if (curtok.type == TOK_OP && (curtok.op == OP_INC || curtok.op == OP_DEC))
    {
        int op = curtok.op;
        next_token();
        if (curtok.type != TOK_IDENT)
            die("expected identifier after ++/--");

        int li = local_find(curtok.ident);
        if (li < 0)
            die("unknown variable");

        if (locals[li].is_const)
            die("increment/decrement of const variable");

        int offset = locals[li].offset;

        emit_mov_eax_from_ebp_disp_any(neg_off(offset));
        if (op == OP_INC)
        {
            int inc = (locals[li].ptr_depth > 0) ? 4 : 1;
            emit_u8(0x83);
            emit_u8(0xC0);
            emit_u8((uint8_t)inc);
        }
        else
        {
            int dec = (locals[li].ptr_depth > 0) ? 4 : 1;
            emit_u8(0x83);
            emit_u8(0xE8);
            emit_u8((uint8_t)dec);
        }
        emit_mov_ebp_disp_from_eax_any(neg_off(offset));
        emit_push_eax();
        next_token();
        return;
    }

    gen_primary();
}

void gen_mul()
{
    gen_unary();
    while (curtok.type == TOK_SYM && (curtok.sym == '*' || curtok.sym == '/' || curtok.sym == '%'))
    {
        char op = curtok.sym;
        next_token();
        gen_unary();
        emit_pop_ebx();
        emit_pop_eax();
        if (op == '*')
            emit_imul_eax_ebx();

        else if (op == '/')
        {
            emit_cdq();
            emit_idiv_ebx();
        }
        else
        {
            emit_cdq();
            emit_idiv_ebx();
            emit_u8(0x89);
            emit_u8(0xD0);
        }
        emit_push_eax();
    }
}

void gen_add()
{
    gen_mul();
    while (curtok.type == TOK_SYM && (curtok.sym == '+' || curtok.sym == '-'))
    {
        char op = curtok.sym;

        int lhs_was_simple_ident = last_primary_is_simple;
        char lhs_ident_saved[64];
        if (lhs_was_simple_ident)
        {
            strncpy(lhs_ident_saved, last_primary_ident, sizeof(lhs_ident_saved) - 1);
            lhs_ident_saved[sizeof(lhs_ident_saved) - 1] = '\0';
        }

        next_token();

        if (op == '+' && lhs_was_simple_ident && curtok.type == TOK_NUMBER)
        {
            int li = local_find(lhs_ident_saved);
            if (li >= 0 && locals[li].ptr_depth > 0)
            {
                int scaled = curtok.val * 4;
                next_token();
                emit_push_imm32((uint32_t)scaled);

                emit_pop_ebx();
                emit_pop_eax();
                emit_add_eax_ebx();
                emit_push_eax();

                last_primary_is_simple = 0;
                continue;
            }
        }

        gen_mul();

        last_primary_is_simple = 0;

        emit_pop_ebx();
        emit_pop_eax();
        if (op == '+')
            emit_add_eax_ebx();
        else
            emit_sub_eax_ebx();
        emit_push_eax();
    }
}

void gen_shift()
{
    gen_add();
    while (curtok.type == TOK_OP && (curtok.op == OP_SHL || curtok.op == OP_SHR))
    {
        int op = curtok.op;
        next_token();
        gen_add();
        emit_pop_ebx();
        emit_pop_eax();
        emit_u8(0x88);
        emit_u8(0xD9);
        if (op == OP_SHL)
        {
            emit_u8(0xD3);
            emit_u8(0xE0);
        }
        else
        {
            emit_u8(0xD3);
            emit_u8(0xE8);
        }
        emit_push_eax();
    }
}

void gen_bitwise()
{
    gen_cmp();
    while (curtok.type == TOK_OP && (curtok.op == OP_BAND || curtok.op == OP_BOR || curtok.op == OP_BXOR))
    {
        int op = curtok.op;
        next_token();
        gen_cmp();
        emit_pop_ebx();
        emit_pop_eax();
        switch (op)
        {
        case OP_BAND:
            emit_u8(0x21);
            emit_u8(0xD8);
            break;
        case OP_BOR:
            emit_u8(0x09);
            emit_u8(0xD8);
            break;
        case OP_BXOR:
            emit_u8(0x31);
            emit_u8(0xD8);
            break;
        }
        emit_push_eax();
    }
}

void gen_logic_and()
{
    gen_bitwise();
    while (curtok.type == TOK_OP && curtok.op == OP_LAND)
    {
        next_token();
        emit_pop_eax();
        emit_test_eax_eax();
        size_t push_false_jmp = emit_jz_rel32_placeholder();
        gen_bitwise();
        emit_pop_eax();
        emit_test_eax_eax();
        emit_setcc_and_push(0x95);
        size_t jmp_after = emit_jmp_rel32_placeholder();
        patch_rel32_at(push_false_jmp, (uint32_t)(code_len - (push_false_jmp + 4)));
        emit_push_imm32(0);
        patch_rel32_at(jmp_after, (uint32_t)(code_len - (jmp_after + 4)));
    }
}

void gen_logic_or()
{
    gen_logic_and();
    while (curtok.type == TOK_OP && curtok.op == OP_LOR)
    {
        next_token();
        emit_pop_eax();
        emit_test_eax_eax();
        size_t push_true_jmp = emit_jnz_rel32_placeholder();

        gen_logic_and();
        emit_pop_eax();
        emit_test_eax_eax();
        emit_setcc_and_push(0x95);

        size_t jmp_after = emit_jmp_rel32_placeholder();

        patch_rel32_at(push_true_jmp, (uint32_t)(code_len - (push_true_jmp + 4)));
        emit_push_imm32(1);

        patch_rel32_at(jmp_after, (uint32_t)(code_len - (jmp_after + 4)));
    }
}

void gen_ternary()
{
    gen_logic_or();
    if (accept_sym('?'))
    {
        emit_pop_eax();
        emit_test_eax_eax();
        size_t jz_off = emit_jz_rel32_placeholder();
        gen_expr();
        size_t jmp_end = emit_jmp_rel32_placeholder();
        patch_rel32_at(jz_off, (uint32_t)(code_len - (jz_off + 4)));
        expect_sym(':');
        gen_expr();
        patch_rel32_at(jmp_end, (uint32_t)(code_len - (jmp_end + 4)));
        emit_push_eax();
    }
}

void gen_cmp()
{
    gen_shift();

    for (;;)
    {
        int is_cmp = 0;
        int cmp_type = -1;

        if (curtok.type == TOK_OP &&
            (curtok.op == OP_LT || curtok.op == OP_LE ||
             curtok.op == OP_GT || curtok.op == OP_GE ||
             curtok.op == OP_EQ || curtok.op == OP_NE))
        {
            is_cmp = 1;
            switch (curtok.op)
            {
            case OP_LT:
                cmp_type = 0;
                break;
            case OP_LE:
                cmp_type = 1;
                break;
            case OP_GT:
                cmp_type = 2;
                break;
            case OP_GE:
                cmp_type = 3;
                break;
            case OP_EQ:
                cmp_type = 4;
                break;
            case OP_NE:
                cmp_type = 5;
                break;
            default:
                die("unknown comparison operator");
            }
        }
        else if (curtok.type == TOK_SYM)
            if (curtok.sym == '=' || curtok.sym == '!')
                die("single '=' or '!' in expression");

        if (!is_cmp)
            break;

        next_token();

        gen_shift();

        emit_pop_ebx();
        emit_pop_eax();

        emit_u8(0x39);
        emit_u8(0xD8);

        switch (cmp_type)
        {
        case 0:
            emit_setcc_and_push(0x9C);
            break;
        case 1:
            emit_setcc_and_push(0x9E);
            break;
        case 2:
            emit_setcc_and_push(0x9F);
            break;
        case 3:
            emit_setcc_and_push(0x9D);
            break;
        case 4:
            emit_setcc_and_push(0x94);
            break;
        case 5:
            emit_setcc_and_push(0x95);
            break;
        default:
            die("unknown cmp_type");
        }
    }
}

void gen_expr() { gen_assign(); }

void gen_stmt();

void gen_block()
{
    expect_sym('{');
    while (!accept_sym('}'))
        gen_stmt();
}

static size_t curtok_text_len(const Token *t)
{
    if (t->type == TOK_IDENT)
        return strlen(t->ident);

    if (t->type == TOK_NUMBER)
    {
        char tmp[32];
        int n = snprintf(tmp, sizeof(tmp), "%d", t->val);
        return (n > 0) ? (size_t)n : 0;
    }

    if (t->type == TOK_SYM)
        return 1;

    if (t->type == TOK_OP)
    {
        switch (t->op)
        {
        case OP_LAND:
        case OP_LOR:
        case OP_SHL:
        case OP_SHR:
        case OP_EQ:
        case OP_NE:
        case OP_LE:
        case OP_GE:
        case OP_INC:
        case OP_DEC:
            return 2;
        default:
            return 1;
        }
    }
    return 0;
}

static size_t resolve_jump_target(size_t tgt)
{
    int safety = 0;
    while (tgt < code_len && safety++ < 256)
    {
        unsigned char *p = code_buf + tgt;

        if (tgt + 5 <= code_len && p[0] == 0xE9)
        {
            int32_t imm;
            memcpy(&imm, p + 1, 4);

            if (imm == 0)
                break;

            int32_t dest = (int32_t)(tgt + 5) + imm;
            if (dest < 0 || (size_t)dest >= code_len)
                break;

            if ((size_t)dest == tgt)
                break;

            tgt = (size_t)dest;
            continue;
        }

        break;
    }
    return tgt;
}

void gen_stmt()
{
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "typedef") == 0)
    {
        next_token();
        if (curtok.type != TOK_IDENT)
            die("expected type name after typedef");

        if (!is_type_name(curtok.ident))
            die("unsupported typedef base type");

        if (strcmp(curtok.ident, "unsigned") == 0 || strcmp(curtok.ident, "signed") == 0)
        {
            next_token();
            if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
                next_token();
        }
        else
            next_token();

        expect_ident();
        char tname[64];
        strncpy(tname, curtok.ident, 63);
        next_token();
        expect_sym(';');
        add_typedef_name(tname);
        return;
    }
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "return") == 0)
    {
        next_token();
        if (accept_sym(';'))
        {
            emit_u8(0xB8);
            emit_u32(0);
            emit_leave();
            emit_ret_pop_args_uint16((uint16_t)current_fun_argbytes);
            return;
        }
        else
        {
            gen_expr();
            emit_pop_eax();
            expect_sym(';');
            emit_leave();
            emit_ret_pop_args_uint16((uint16_t)current_fun_argbytes);
            return;
        }
    }
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "if") == 0)
    {
        next_token();
        expect_sym('(');
        gen_expr();
        expect_sym(')');
        emit_pop_eax();
        emit_test_eax_eax();

        size_t jz_to_next = emit_jz_rel32_placeholder();

        gen_stmt();

        size_t end_jumps[32];
        int end_jumps_cnt = 0;
        end_jumps[end_jumps_cnt++] = emit_jmp_rel32_placeholder();

        while (curtok.type == TOK_IDENT && strcmp(curtok.ident, "else") == 0)
        {
            next_token();
            if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "if") == 0)
            {
                next_token();
                expect_sym('(');

                size_t cond_start = resolve_jump_target(code_len);

                gen_expr();
                expect_sym(')');
                emit_pop_eax();
                emit_test_eax_eax();

                size_t new_jz = emit_jz_rel32_placeholder();

                patch_rel32_at(jz_to_next, (uint32_t)(cond_start - (jz_to_next + 4)));
                jz_to_next = new_jz;

                gen_stmt();

                if (end_jumps_cnt < (int)(sizeof(end_jumps) / sizeof(end_jumps[0])))
                    end_jumps[end_jumps_cnt++] = emit_jmp_rel32_placeholder();
                else
                    die("too many else-if branches");
            }

            else
            {
                patch_rel32_at(jz_to_next, (uint32_t)(resolve_jump_target(code_len) - (jz_to_next + 4)));
                gen_stmt();
                jz_to_next = (size_t)-1;
                break;
            }
        }

        if (jz_to_next != (size_t)-1)
            patch_rel32_at(jz_to_next, (uint32_t)(resolve_jump_target(code_len) - (jz_to_next + 4)));

        for (int i = 0; i < end_jumps_cnt; ++i)
        {
            size_t place = end_jumps[i];
            patch_rel32_at(place, (uint32_t)(code_len - (place + 4)));
        }

        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "do") == 0)
    {
        next_token();

        if (loop_depth >= MAX_LOOP_DEPTH)
            die("loop depth too big");

        size_t body_start = code_len;
        loop_start[loop_depth] = body_start;
        break_counts[loop_depth] = 0;
        continue_counts[loop_depth] = 0;
        loop_depth++;

        gen_stmt();

        if (!(curtok.type == TOK_IDENT && strcmp(curtok.ident, "while") == 0))
            die("expected 'while' after 'do' body");

        next_token();
        expect_sym('(');

        size_t cond_pos = code_len;
        gen_expr();
        expect_sym(')');
        expect_sym(';');

        emit_pop_eax();
        emit_test_eax_eax();
        size_t jnz_off = emit_jnz_rel32_placeholder();

        uint32_t rel_back = (uint32_t)((int32_t)body_start - (int32_t)(jnz_off + 4));
        patch_rel32_at(jnz_off, rel_back);

        for (int i = 0; i < break_counts[loop_depth - 1]; i++)
            patch_rel32_at(break_addrs[loop_depth - 1][i], (uint32_t)(code_len - (break_addrs[loop_depth - 1][i] + 4)));

        for (int i = 0; i < continue_counts[loop_depth - 1]; i++)
        {
            size_t place = continue_addrs[loop_depth - 1][i];
            int32_t rel = (int32_t)((int32_t)cond_pos - (int32_t)(place + 4));
            patch_rel32_at(place, (uint32_t)rel);
        }

        loop_depth--;
        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "for") == 0)
    {
        next_token();
        expect_sym('(');

        if (!accept_sym(';'))
        {
            gen_expr();
            emit_pop_eax();
            expect_sym(';');
        }

        size_t cond_pos = code_len;
        size_t jz_off = 0;

        if (!accept_sym(';'))
        {
            gen_expr();
            emit_pop_eax();
            emit_test_eax_eax();
            jz_off = emit_jz_rel32_placeholder();
            expect_sym(';');
        }

        char incr_buf[2048];
        incr_buf[0] = '\0';
        size_t incr_len = 0;
        int has_incr = 0;

        if (!(curtok.type == TOK_SYM && curtok.sym == ')'))
        {
            size_t first_tok_len = curtok_text_len(&curtok);

            ptrdiff_t cur_offset = (ptrdiff_t)(src - (char *)src_buf);
            ptrdiff_t start_offset = cur_offset - (ptrdiff_t)first_tok_len;

            char *start;
            if (start_offset < 0)
                start = (char *)src_buf;
            else
                start = (char *)(src_buf + start_offset);

            start = skip_ws(start);

            char *p = start;
            int depth = 0;
            while (*p)
            {
                if (*p == '(')
                    depth++;
                else if (*p == ')')
                {
                    if (depth == 0)
                        break;
                    depth--;
                }
                p++;
            }
            if (!*p)
                die("unterminated for() header");

            incr_len = (size_t)(p - start);
            if (incr_len >= sizeof(incr_buf))
                die("for-increment too long");

            memcpy(incr_buf, start, incr_len);
            incr_buf[incr_len] = '\0';

            src = p + 1;
            next_token();
            has_incr = (incr_len > 0);
        }
        else if (curtok.type == TOK_SYM && curtok.sym == ')')
            next_token();

        if (loop_depth >= MAX_LOOP_DEPTH)
            die("loop depth too big");

        loop_start[loop_depth] = cond_pos;
        loop_cond[loop_depth] = cond_pos;
        break_counts[loop_depth] = 0;
        continue_counts[loop_depth] = 0;
        loop_depth++;

        gen_stmt();

        size_t incr_start = cond_pos;
        if (has_incr)
        {
            Token saved_tok = curtok;
            char *saved_src = src;

            src = incr_buf;
            next_token();

            incr_start = code_len;
            gen_expr();
            emit_pop_eax();

            curtok = saved_tok;
            src = saved_src;
        }
        else
            incr_start = cond_pos;

        size_t jmp_back = emit_jmp_rel32_placeholder();
        {
            int32_t rel_back = (int32_t)((int32_t)cond_pos - (int32_t)(jmp_back + 4));
            patch_rel32_at(jmp_back, (uint32_t)rel_back);
        }

        for (int i = 0; i < break_counts[loop_depth - 1]; i++)
            patch_rel32_at(break_addrs[loop_depth - 1][i],
                           (uint32_t)(code_len - (break_addrs[loop_depth - 1][i] + 4)));

        for (int i = 0; i < continue_counts[loop_depth - 1]; i++)
        {
            size_t place = continue_addrs[loop_depth - 1][i];
            int32_t rel = (int32_t)((int32_t)incr_start - (int32_t)(place + 4));
            patch_rel32_at(place, (uint32_t)rel);
        }

        loop_depth--;

        if (jz_off)
            patch_rel32_at(jz_off, (uint32_t)(code_len - (jz_off + 4)));

        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "break") == 0)
    {
        next_token();
        expect_sym(';');
        if (loop_depth <= 0)
            die("break not in loop");

        size_t off = emit_jmp_rel32_placeholder();
        if (break_counts[loop_depth - 1] >= 128)
            die("too many break statements in one loop");

        break_addrs[loop_depth - 1][break_counts[loop_depth - 1]++] = off;
        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "continue") == 0)
    {
        next_token();
        expect_sym(';');
        if (loop_depth <= 0)
            die("continue not in loop");

        size_t off = emit_jmp_rel32_placeholder();
        if (continue_counts[loop_depth - 1] >= 128)
            die("too many continue statements in one loop");

        continue_addrs[loop_depth - 1][continue_counts[loop_depth - 1]++] = off;
        return;
    }
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "while") == 0)
    {
        next_token();
        expect_sym('(');
        size_t cond_pos = code_len;
        gen_expr();
        expect_sym(')');
        emit_pop_eax();
        emit_test_eax_eax();
        size_t jz_off = emit_jz_rel32_placeholder();

        if (loop_depth >= MAX_LOOP_DEPTH)
            die("loop depth too big");

        loop_start[loop_depth] = cond_pos;
        loop_cond[loop_depth] = cond_pos;
        break_counts[loop_depth] = 0;
        continue_counts[loop_depth] = 0;
        loop_depth++;

        gen_stmt();

        size_t jmp_back = emit_jmp_rel32_placeholder();
        uint32_t to_back = (uint32_t)((int32_t)cond_pos - (int32_t)(jmp_back + 4));
        patch_rel32_at(jmp_back, to_back);

        for (int i = 0; i < break_counts[loop_depth - 1]; i++)
            patch_rel32_at(break_addrs[loop_depth - 1][i], (uint32_t)(code_len - (break_addrs[loop_depth - 1][i] + 4)));
        for (int i = 0; i < continue_counts[loop_depth - 1]; i++)
            patch_rel32_at(continue_addrs[loop_depth - 1][i], (uint32_t)((int32_t)cond_pos - (int32_t)(continue_addrs[loop_depth - 1][i] + 4)));

        loop_depth--;

        uint32_t rel = (uint32_t)(code_len - (jz_off + 4));
        patch_rel32_at(jz_off, rel);
        return;
    }
    if (accept_sym('{'))
    {
        while (!accept_sym('}'))
            gen_stmt();

        return;
    }
    if (curtok.type == TOK_IDENT && (strcmp(curtok.ident, "const") == 0 || is_type_name(curtok.ident) || strcmp(curtok.ident, "static") == 0 || strcmp(curtok.ident, "auto") == 0))
    {
        int is_const = 0;
        if (strcmp(curtok.ident, "static") == 0 || strcmp(curtok.ident, "auto") == 0)
            next_token();

        if (strcmp(curtok.ident, "const") == 0)
        {
            is_const = 1;
            next_token();
            if (!(curtok.type == TOK_IDENT && is_type_name(curtok.ident)))
                die("expected type after 'const'");
        }

        if (!(curtok.type == TOK_IDENT && is_type_name(curtok.ident)))
            die("expected type in declaration");

        int struct_id = -1;
        if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
        {
            int is_union = (strcmp(curtok.ident, "union") == 0);
            next_token();
            expect_ident();
            struct_id = struct_find(curtok.ident);
            if (struct_id < 0)
                die("unknown struct/union type");
            next_token();
        }
        else if (strcmp(curtok.ident, "unsigned") == 0 || strcmp(curtok.ident, "signed") == 0)
        {
            next_token();
            if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
                next_token();
        }
        else
            next_token();

        int ptr_depth = 0;
        while (accept_sym('*'))
            ptr_depth++;

        expect_ident();
        char vname[64];
        strncpy(vname, curtok.ident, 63);
        vname[63] = '\0';
        next_token();

        int base = local_stack_size;
        int li = alloc_local(vname, is_const);
        locals[li].ptr_depth = ptr_depth;
        locals[li].struct_id = struct_id;

        int arrsz = 0;
        int elem_size = 4;
        if (struct_id >= 0 && ptr_depth == 0)
            elem_size = structsyms[struct_id].size;

        if (accept_sym('['))
        {
            if (curtok.type != TOK_NUMBER)
                die("expected number in array size");
            arrsz = curtok.val;
            next_token();
            expect_sym(']');

            if (arrsz <= 0)
                die("invalid array size");

            int total = elem_size * arrsz;
            if (total < 4)
                total = 4;

            locals[li].arr_size = arrsz;
            locals[li].offset = base + total;
            local_stack_size = base + total;
            emit_sub_esp_imm((uint32_t)total);
        }
        else
        {
            int total = elem_size;
            if (total < 4)
                total = 4;

            locals[li].offset = base + total;
            local_stack_size = base + total;
            emit_sub_esp_imm((uint32_t)total);
        }

        if (accept_sym('='))
        {
            if (locals[li].arr_size > 0)
                die("array initialization not supported");
            gen_expr();
            emit_pop_eax();
            emit_mov_ebp_disp_from_eax_any(neg_off(locals[li].offset));
        }
        else if (is_const)
            die("const variable must be initialized");

        expect_sym(';');
        return;
    }

    if (curtok.type == TOK_IDENT)
    {
        Token save = curtok;
        char *save_src = src;
        char name[64];
        strncpy(name, curtok.ident, 63);

        next_token();

        if (curtok.type == TOK_SYM && curtok.sym == '=')
        {
            next_token();
            gen_expr();
            int li = local_find(name);
            if (li < 0)
                die("unknown variable on assign");

            emit_pop_eax();
            emit_mov_ebp_disp_from_eax_any(neg_off(locals[li].offset));
            expect_sym(';');
            return;
        }

        curtok = save;
        src = save_src;

        gen_expr();
        expect_sym(';');
        emit_pop_eax();

        return;
    }
    gen_expr();
    expect_sym(';');
    emit_pop_eax();
}

void gen_function()
{
    int is_void = 0;

    if (!(curtok.type == TOK_IDENT && is_type_name(curtok.ident)))
        die("function must start with a type (int/void or typedef)");
    if (strcmp(curtok.ident, "void") == 0)
        is_void = 1;

    next_token();

    expect_ident();
    char fname[64];
    strncpy(fname, curtok.ident, 63);
    next_token();
    expect_sym('(');
    int argc = 0;
    char params[16][64];

    if (!accept_sym(')'))
    {
        while (1)
        {
            if (curtok.type != TOK_IDENT || !is_type_name(curtok.ident))
                die("expected type in parameter");

            if (strcmp(curtok.ident, "unsigned") == 0 || strcmp(curtok.ident, "signed") == 0)
            {
                next_token();
                if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
                    next_token();
            }
            else
            {
                next_token();
            }

            while (accept_sym('*'))
                ;

            expect_ident();
            strncpy(params[argc], curtok.ident, 63);
            params[argc][63] = '\0';
            next_token();
            argc++;

            if (accept_sym(')'))
                break;

            expect_sym(',');
        }
    }

    uint32_t func_offset = (uint32_t)code_len;
    if (funcs_cnt >= MAX_FUNCS)
        die("too many functions");

    strncpy(funcs[funcs_cnt].name, fname, sizeof(funcs[funcs_cnt].name) - 1);
    funcs[funcs_cnt].name[sizeof(funcs[funcs_cnt].name) - 1] = '\0';

    funcs[funcs_cnt].offset = func_offset;
    funcs[funcs_cnt].argc = argc;
    funcs_cnt++;

    emit_u8(0x55);
    emit_u8(0x89);
    emit_u8(0xE5);

    locals_cnt = 0;
    local_stack_size = 0;
    current_fun_argbytes = argc * 4;

    for (int i = 0; i < argc; i++)
        alloc_local(params[i], 0);

    if (local_stack_size > 0)
        emit_sub_esp_imm(local_stack_size);

    for (int i = 0; i < argc; i++)
    {
        int li = i;
        int param_src_disp = 8 + i * 4;
        emit_u8(0x8B);
        emit_u8(0x45);
        emit_u8((uint8_t)param_src_disp);
        emit_mov_ebp_disp_from_eax_any(neg_off(locals[li].offset));
    }

    expect_sym('{');
    while (!accept_sym('}'))
        gen_stmt();

    emit_u8(0xB8);
    emit_u32(0);
    emit_leave();
    emit_ret_pop_args_uint16((uint16_t)current_fun_argbytes);
}

void compile(const char *input)
{
    src = (char *)input;
    next_token();

    while (curtok.type != TOK_EOF)
    {
        if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "typedef") == 0)
        {
            next_token();

            if (curtok.type != TOK_IDENT)
                die("expected type name after typedef");

            if (!is_type_name(curtok.ident))
                die("unsupported typedef base type");

            if (strcmp(curtok.ident, "unsigned") == 0 || strcmp(curtok.ident, "signed") == 0)
            {
                next_token();
                if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
                    next_token();
            }
            else
                next_token();

            expect_ident();
            char tname[64];
            strncpy(tname, curtok.ident, sizeof(tname) - 1);
            tname[sizeof(tname) - 1] = '\0';
            next_token();

            expect_sym(';');

            add_typedef_name(tname);
            continue;
        }

        if (curtok.type == TOK_IDENT && (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0))
        {
            int is_union = (strcmp(curtok.ident, "union") == 0);
            next_token();

            char sname[64] = "";
            if (curtok.type == TOK_IDENT)
            {
                strncpy(sname, curtok.ident, sizeof(sname) - 1);
                sname[sizeof(sname) - 1] = '\0';
                next_token();
            }

            if (accept_sym('{'))
            {
                int si = -1;
                if (sname[0] != '\0')
                    si = add_structsym(sname, is_union);
                else
                {
                    char tmpn[64];
                    snprintf(tmpn, sizeof(tmpn), "__anon_struct_%d", structsyms_cnt);
                    si = add_structsym(tmpn, is_union);
                }

                while (!accept_sym('}'))
                {
                    if (!(curtok.type == TOK_IDENT && is_type_name(curtok.ident)))
                        die("expected type in struct field");
                    next_token();

                    int ptr_depth = 0;
                    while (accept_sym('*'))
                        ptr_depth++;

                    expect_ident();
                    char fname[64];
                    strncpy(fname, curtok.ident, 63);
                    fname[63] = '\0';
                    next_token();

                    int fsize = 4;
                    if (ptr_depth > 0)
                        fsize = 4;
                    else if (accept_sym('['))
                    {
                        if (curtok.type != TOK_NUMBER)
                            die("expected number in array size");
                        int arrsz = curtok.val;
                        next_token();
                        expect_sym(']');
                        fsize = arrsz * 4;
                    }

                    add_struct_field(si, fname, fsize);

                    expect_sym(';');
                }

                if (curtok.type == TOK_IDENT)
                {
                    char tname[64];
                    strncpy(tname, curtok.ident, 63);
                    tname[63] = '\0';
                    next_token();
                    expect_sym(';');

                    add_typedef_name(tname);

                    int existing = struct_find(tname);
                    if (existing < 0)
                    {
                        if (structsyms_cnt >= MAX_STRUCTS)
                            die("too many structs");
                        int newidx = add_structsym(tname, is_union);
                        StructSym *srcs = &structsyms[si];
                        StructSym *dsts = &structsyms[newidx];
                        dsts->field_cnt = 0;
                        for (int fi = 0; fi < srcs->field_cnt; ++fi)
                            add_struct_field(newidx, srcs->field_names[fi], srcs->field_sizes[fi]);
                    }
                }
                else
                    expect_sym(';');

                continue;
            }
            else
            {
                expect_sym(';');
                continue;
            }
        }

        if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "enum") == 0)
        {
            next_token();
            if (curtok.type == TOK_IDENT)
                next_token();

            if (accept_sym('{'))
            {
                int32_t val = 0;
                while (!accept_sym('}'))
                {
                    expect_ident();
                    char ename[64];
                    strncpy(ename, curtok.ident, 63);
                    ename[63] = '\0';
                    next_token();
                    if (accept_sym('='))
                    {
                        if (curtok.type == TOK_NUMBER)
                        {
                            val = curtok.val;
                            next_token();
                        }
                        else
                            die("expected number in enum");
                    }
                    add_enum_const(ename, val);
                    val++;
                    accept_sym(',');
                }
                expect_sym(';');
                continue;
            }
            else
            {
                expect_sym(';');
                continue;
            }
        }

        gen_function();
    }
}

typedef struct
{
    unsigned char e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
    uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} Elf32_Phdr;

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "\033[1;33mUsage: baoc <input.c> <output.elf>\033[0m\n");
        return 1;
    }

    if (strcmp(argv[1], argv[2]) == 0)
    {
        fprintf(stderr, "\033[31mError: output file cannot be the same as input file.\033[0m\n");
        return 1;
    }

    const char *crt0_path = "/lib/crt0";

    FILE *f = fopen(argv[1], "rb");
    if (!f)
        die("cannot open input");
    size_t src_sz = fread(src_buf, 1, MAX_FILE_SIZE, f);
    fclose(f);

    if (src_sz >= MAX_FILE_SIZE)
        src_sz = MAX_FILE_SIZE - 1;
    src_buf[src_sz] = '\0';

    code_len = 0;
    relocs_cnt = 0;
    funcs_cnt = 0;

    static unsigned char preproc_buf[(MAX_FILE_SIZE * 3)];
    preprocess_src(src_buf, preproc_buf, sizeof(preproc_buf));

    FILE *logs = fopen("baoc.log", "w");

    fprintf(logs, "=== Preprocessed Source ===\n%s\n=== End Preprocessed Source ===\n", preproc_buf);
    fclose(logs);

    compile((const char *)preproc_buf);

    FILE *fcrt = fopen(crt0_path, "rb");
    if (!fcrt)
        die("cannot open crt0");
    fseek(fcrt, 0, SEEK_END);
    long crt_sz = ftell(fcrt);
    fseek(fcrt, 0, SEEK_SET);
    if (crt_sz <= 0 || crt_sz > MAX_FILE_SIZE)
        die("crt0 size invalid");
    fread(crt_buf, 1, crt_sz, fcrt);
    fclose(fcrt);

    uint32_t user_code_size = (uint32_t)code_len;

    uint32_t total_str_size = 0;
    for (int i = 0; i < strlits_cnt; ++i)
        total_str_size += (uint32_t)(strlen(strlits[i].s) + 1);

    uint32_t total_user_size = user_code_size + total_str_size;

    uint32_t ph_offset = sizeof(Elf32_Ehdr);
    uint32_t elf_size = ph_offset + sizeof(Elf32_Phdr) + crt_sz + total_user_size;
    if ((size_t)elf_size > MAX_IMAGE)
        die("output too large");
    memset(image_buf, 0, elf_size);

    Elf32_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0] = 0x7F;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 1;
    ehdr.e_ident[5] = 1;
    ehdr.e_ident[6] = 1;
    ehdr.e_type = 2;
    ehdr.e_machine = 3;
    ehdr.e_version = 1;
    ehdr.e_entry = LOAD_BASE;
    ehdr.e_phoff = ph_offset;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_phnum = 1;

    memcpy(image_buf, &ehdr, sizeof(Elf32_Ehdr));

    Elf32_Phdr ph;
    ph.p_type = 1;
    ph.p_offset = ph_offset + sizeof(Elf32_Phdr);
    ph.p_vaddr = LOAD_BASE;
    ph.p_paddr = LOAD_BASE;
    ph.p_filesz = crt_sz + total_user_size;
    ph.p_memsz = crt_sz + total_user_size;
    ph.p_flags = 7;
    ph.p_align = 0x1000;

    memcpy(image_buf + ph_offset, &ph, sizeof(ph));

    memcpy(image_buf + ph_offset + sizeof(Elf32_Phdr), crt_buf, crt_sz);
    memcpy(image_buf + ph_offset + sizeof(Elf32_Phdr) + crt_sz, code_buf, user_code_size);

    uint32_t str_data_file_off = ph_offset + sizeof(Elf32_Phdr) + (uint32_t)crt_sz + user_code_size;
    uint32_t str_data_vaddr = ph.p_vaddr + (uint32_t)crt_sz + user_code_size;
    uint32_t cur_file_off = str_data_file_off;
    uint32_t cur_vaddr = str_data_vaddr;
    for (int i = 0; i < strlits_cnt; ++i)
    {
        size_t len = strlen(strlits[i].s) + 1;
        if ((size_t)(cur_file_off + len) > (size_t)elf_size)
            die("string pool out of bounds");

        memcpy(image_buf + cur_file_off, strlits[i].s, len);

        uint32_t imm_file_loc = ph_offset + sizeof(Elf32_Phdr) + (uint32_t)crt_sz + strlits[i].code_place;
        if (imm_file_loc + 4u > (uint32_t)elf_size)
            die("string literal imm out of bounds");

        uint32_t code_opcode_file = ph_offset + sizeof(Elf32_Phdr) + (uint32_t)crt_sz + (strlits[i].code_place - 1);
        if (strlits[i].code_place == 0 || image_buf[code_opcode_file] != 0x68)
            die("expected push imm at recorded place");

        uint32_t target_vaddr = cur_vaddr;
        memcpy(image_buf + imm_file_loc, &target_vaddr, 4);

        cur_file_off += (uint32_t)len;
        cur_vaddr += (uint32_t)len;
    }

    uint32_t seg_file_base = ph_offset + sizeof(Elf32_Phdr);
    uint32_t seg_vaddr = ph.p_vaddr;
    for (int r = 0; r < relocs_cnt; r++)
    {
        int fi = func_find(relocs[r].name);
        if (fi < 0)
            die("call to unknown function (final patch)");

        uint32_t func_file_off = funcs[fi].offset;
        uint32_t func_virt = seg_vaddr + (uint32_t)crt_sz + func_file_off;

        uint32_t call_site = relocs[r].call_site;
        if (call_site == 0)
            die("invalid call site");

        uint32_t imm_file_loc = seg_file_base + (uint32_t)crt_sz + call_site;
        if (imm_file_loc + 4u > (uint32_t)elf_size)
            die("reloc imm out of bounds");

        uint32_t call_instr_virt = seg_vaddr + (uint32_t)crt_sz + (call_site - 1u);

        uint32_t diff = func_virt - (call_instr_virt + 5u);
        int32_t rel32 = (int32_t)diff;

        memcpy(image_buf + imm_file_loc, &rel32, 4);
    }

    int call_offset_in_crt = -1;
    for (int i = 0; i + 4 < crt_sz; ++i)
        if (crt_buf[i] == 0xE8)
        {
            call_offset_in_crt = i;
            break;
        }

    if (call_offset_in_crt >= 0)
    {
        uint32_t call_place_in_file = ph_offset + sizeof(Elf32_Phdr) + (uint32_t)call_offset_in_crt;
        uint32_t call_instr_virt = LOAD_BASE + (uint32_t)call_offset_in_crt;
        uint32_t main_target_virt = LOAD_BASE + (uint32_t)crt_sz;

        int32_t rel32 = (int32_t)(main_target_virt - (call_instr_virt + 5u));
        memcpy(image_buf + call_place_in_file + 1, &rel32, 4);

        int main_idx = func_find("main");
        if (main_idx >= 0)
        {
            uint32_t main_offset = funcs[main_idx].offset;
            if (main_idx >= 0)
                for (int i = (int)code_len - 1; i >= (int)main_offset; i--)

                    if (code_buf[i] == 0xC2 && i + 2 < (int)code_len)
                    {
                        uint16_t imm = 8;
                        size_t dest_off = ph_offset + sizeof(Elf32_Phdr) + crt_sz + i + 1;
                        memcpy(image_buf + dest_off, &imm, 2);
                        break;
                    }
        }
    }
    else
        fprintf(stderr, "\033[1;33mWarning: no CALL opcode found in crt0; crt0 might not contain 'call main'.\033[0m\n");

    FILE *out = fopen(argv[2], "wb");
    if (!out)
        die("cannot open output");

    fwrite(image_buf, 1, elf_size, out);
    fclose(out);

    printf("\033[32mWrote %s (%u bytes) as ELF (simple subset compiler, no libc).\033[0m\n", argv[2], elf_size);
    return 0;
}
