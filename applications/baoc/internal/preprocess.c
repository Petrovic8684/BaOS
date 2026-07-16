#include "baoc_internal.h"

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

void pp_macros_clear_all(void)
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


void pp_macro_undef(const char *name)
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


void pp_macro_add_object(const char *name, const char *body)
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


void pp_macro_add_function(const char *name, char params[][PP_MAX_MACRO_PARAM_LEN], int pcount, const char *body)
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


int preclean_input_into(const unsigned char *in, unsigned char *out, size_t out_sz)
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


long read_file_into(const char *path, unsigned char *dest, size_t dest_max)
{
    if (!path || !dest || dest_max == 0)
        return -1;

    size_t got = 0;
    if (file_read_all(path, dest, dest_max, &got) != 0)
        return -1;

    if (got >= dest_max)
        got = dest_max - 1;

    dest[got] = '\0';
    return (long)got;
}


int pp_is_ident_char(char c, int first)
{
    if (first)
        return (isalpha((unsigned char)c) || c == '_');
    return (isalnum((unsigned char)c) || c == '_');
}


int parse_macro_args_into(const char *p, char args[PP_ARG_MAX][PP_ARG_LEN], int *out_argc, const char **p_after)
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


int pp_macro_instantiate_to(PP_Macro *m, char args[PP_ARG_MAX][PP_ARG_LEN], int argc, char *outbuf, size_t outbuf_sz)
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


int expand_macros_line_to(const char *line, char *outbuf, size_t outbuf_sz, int depth)
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


void trim_inplace(char *s)
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


static const char *pp_skip_ws(const char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

static int pp_parse_primary(const char **pexpr);
static int pp_parse_unary(const char **pexpr);
static int pp_parse_cmp(const char **pexpr);
static int pp_parse_land(const char **pexpr);
static int pp_parse_lor(const char **pexpr);

static int pp_defined_expr(const char **pexpr)
{
    const char *p = *pexpr;
    p = pp_skip_ws(p);
    if (strncmp(p, "defined", 7) != 0)
        return -1;
    p += 7;
    p = pp_skip_ws(p);
    char name[PP_MAX_MACRO_NAME];
    if (*p == '(')
    {
        p++;
        int i = 0;
        while (*p && *p != ')' && i + 1 < (int)sizeof(name))
            name[i++] = *p++;
        name[i] = '\0';
        if (*p == ')')
            p++;
    }
    else
    {
        int i = 0;
        while (*p && (isalnum((unsigned char)*p) || *p == '_') && i + 1 < (int)sizeof(name))
            name[i++] = *p++;
        name[i] = '\0';
    }
    trim_inplace(name);
    *pexpr = p;
    return pp_macro_find(name) ? 1 : 0;
}

static int pp_parse_primary(const char **pexpr)
{
    const char *p = pp_skip_ws(*pexpr);
    if (*p == '(')
    {
        p++;
        *pexpr = p;
        int v = pp_parse_lor(pexpr);
        p = pp_skip_ws(*pexpr);
        if (*p == ')')
            p++;
        *pexpr = p;
        return v;
    }
    if (strncmp(p, "defined", 7) == 0)
    {
        *pexpr = p;
        return pp_defined_expr(pexpr);
    }
    if (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)p[1])))
    {
        char *end;
        long v = strtol(p, &end, 0);
        *pexpr = end;
        return v != 0;
    }
    return 0;
}

static int pp_parse_unary(const char **pexpr)
{
    const char *p = pp_skip_ws(*pexpr);
    if (*p == '!')
    {
        *pexpr = p + 1;
        return !pp_parse_unary(pexpr);
    }
    return pp_parse_primary(pexpr);
}

static int pp_parse_cmp(const char **pexpr)
{
    int v = pp_parse_unary(pexpr);
    const char *p = pp_skip_ws(*pexpr);
    if (strncmp(p, "==", 2) == 0)
    {
        p += 2;
        int r = pp_parse_unary(&p);
        *pexpr = p;
        return v == r;
    }
    if (strncmp(p, "!=", 2) == 0)
    {
        p += 2;
        int r = pp_parse_unary(&p);
        *pexpr = p;
        return v != r;
    }
    if (*p == '<')
    {
        p++;
        int r = pp_parse_unary(&p);
        *pexpr = p;
        return v < r;
    }
    if (*p == '>')
    {
        p++;
        int r = pp_parse_unary(&p);
        *pexpr = p;
        return v > r;
    }
    *pexpr = p;
    return v;
}

static int pp_parse_land(const char **pexpr)
{
    int v = pp_parse_cmp(pexpr);
    for (;;)
    {
        const char *p = pp_skip_ws(*pexpr);
        if (strncmp(p, "&&", 2) != 0)
            break;
        p += 2;
        int r = pp_parse_cmp(&p);
        v = v && r;
        *pexpr = p;
    }
    return v;
}

static int pp_parse_lor(const char **pexpr)
{
    int v = pp_parse_land(pexpr);
    for (;;)
    {
        const char *p = pp_skip_ws(*pexpr);
        if (strncmp(p, "||", 2) != 0)
            break;
        p += 2;
        int r = pp_parse_land(&p);
        v = v || r;
        *pexpr = p;
    }
    return v;
}

int eval_simple_if_expr_pp(const char *expr)
{
    const char *p = expr;
    return pp_parse_lor(&p) ? 1 : 0;
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

void preprocess_src(const unsigned char *in, unsigned char *out, size_t out_max)
{
    if (!in || !out)
        die("preprocess_src: bad args");

    unsigned char *clean_buf = (unsigned char *)malloc(PP_FILE_BUF_SIZE);
    unsigned char *inc_buf = (unsigned char *)malloc((size_t)PP_MAX_DEPTH * PP_FILE_BUF_SIZE);
    unsigned char *tmp_clean = (unsigned char *)malloc(PP_FILE_BUF_SIZE);
    char *expanded = (char *)malloc(PP_OUT_MAX);

    if (!clean_buf || !inc_buf || !tmp_clean || !expanded)
    {
        free(clean_buf);
        free(inc_buf);
        free(tmp_clean);
        free(expanded);
        die("out of memory");
    }

    pp_macros_clear_all();

    if (preclean_input_into(in, clean_buf, PP_FILE_BUF_SIZE) < 0)
        die("preclean_input failed");

    static char include_stack[PP_MAX_DEPTH][512];
    const char *cur_pos[PP_MAX_DEPTH];

    strncpy((char *)inc_buf, (const char *)clean_buf, PP_FILE_BUF_SIZE - 1);
    inc_buf[PP_FILE_BUF_SIZE - 1] = '\0';
    include_stack[0][0] = '\0';
    cur_pos[0] = (const char *)inc_buf;

    int current_depth = 0;
    size_t out_len = 0;

    int cond_stack[PP_MAX_DEPTH];
    int cond_top = 0;
    cond_stack[0] = 1;
    cond_top = 1;

    while (current_depth >= 0)
    {
        unsigned char *curbuf = inc_buf + (size_t)current_depth * PP_FILE_BUF_SIZE;
        const char *p = cur_pos[current_depth];

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
                            continue;
                        }
                        unsigned char *next_inc = inc_buf + (size_t)(current_depth + 1) * PP_FILE_BUF_SIZE;
                        long r = read_file_into(pathbuf, next_inc, PP_FILE_BUF_SIZE);
                        if (r < 0)
                        {
                            fprintf(stderr, "preprocessor error: failed to read include file: \"%s\" (from %s)\n",
                                    pathbuf,
                                    include_stack[current_depth][0] ? include_stack[current_depth] : "<input>");
                            die("include read failed");
                        }
                        if (preclean_input_into(next_inc, tmp_clean, PP_FILE_BUF_SIZE) < 0)
                            die("preclean_input failed");
                        strncpy((char *)next_inc, (const char *)tmp_clean, PP_FILE_BUF_SIZE - 1);
                        next_inc[PP_FILE_BUF_SIZE - 1] = '\0';
                        strncpy(include_stack[current_depth + 1], pathbuf, sizeof(include_stack[0]) - 1);
                        include_stack[current_depth + 1][sizeof(include_stack[0]) - 1] = '\0';
                        current_depth++;
                        cur_pos[current_depth] = (const char *)(inc_buf + (size_t)current_depth * PP_FILE_BUF_SIZE);
                        p = cur_pos[current_depth];
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
                else if (strncmp(s, "error", 5) == 0 && isspace((unsigned char)s[5]))
                {
                    fprintf(stderr, "\033[31mPreprocessor #error: %s\033[0m\n", s + 5);
                    die("preprocessor #error");
                }
                else if (strncmp(s, "pragma", 6) == 0 && isspace((unsigned char)s[6]))
                {
                    continue;
                }
                else
                {
                    continue;
                }
            }

            if (!cond_stack[cond_top - 1])
                continue;

            if (expand_macros_line_to(linebuf, expanded, PP_OUT_MAX, 0) != 0)
                die("macro expansion error");
            size_t elen = strlen(expanded);
            if (out_len + elen + 2 > out_max)
                die("preprocessed source too large");
            memcpy(out + out_len, expanded, elen);
            out_len += elen;
            out[out_len++] = '\n';
            out[out_len] = '\0';
        }

        if (current_depth == 0)
            break;
        include_stack[current_depth][0] = '\0';
        current_depth--;
    }

    if (out_len >= out_max)
        die("preprocess output overflow");
    out[out_len] = '\0';

    free(clean_buf);
    free(inc_buf);
    free(tmp_clean);
    free(expanded);
}
