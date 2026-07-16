#include "baoc_internal.h"

void gen_function()
{
    int is_void = 0;
    int ret_ptr_depth = 0;
    int ret_struct_id = -1;

    if (!(curtok.type == TOK_IDENT && (is_type_name(curtok.ident) || strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)))
        die("function must start with a type (int/void/typedef/struct)");

    if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
    {
        int is_union = (strcmp(curtok.ident, "union") == 0);
        next_token();
        expect_ident();
        ret_struct_id = struct_find(curtok.ident);
        if (ret_struct_id < 0)
            die("unknown struct/union type in function return");
        next_token();
    }
    else
    {
        if (strcmp(curtok.ident, "void") == 0)
            is_void = 1;
        next_token();
    }

    while (accept_sym('*'))
        ret_ptr_depth++;

    expect_ident();
    char fname[64];
    strncpy(fname, curtok.ident, 63);
    fname[63] = '\0';
    strncpy(current_func_name, fname, sizeof(current_func_name) - 1);
    current_func_name[sizeof(current_func_name) - 1] = '\0';
    next_token();

    expect_sym('(');

    int argc = 0;
    char params[16][64];
    int param_ptr_depth[16];
    int param_struct_id[16];
    int is_variadic = 0;

    if (!accept_sym(')'))
    {
        while (1)
        {
            if (curtok.type == TOK_SYM && curtok.sym == '.' && src[0] == '.' && src[1] == '.')
            {
                src += 2;
                next_token();
                is_variadic = 1;
                if (!accept_sym(')'))
                    die("expected ')' after '...'");
                break;
            }

            if (curtok.type != TOK_IDENT || !is_type_name(curtok.ident))
                die("expected type in parameter");

            int this_struct_id = -1;
            if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
            {
                int is_union = (strcmp(curtok.ident, "union") == 0);
                next_token();
                expect_ident();
                this_struct_id = struct_find(curtok.ident);
                if (this_struct_id < 0)
                    die("unknown struct/union type in parameter");
                next_token();
            }
            else
            {
                next_token();
            }

            int ptrd = 0;
            while (accept_sym('*'))
                ptrd++;

            expect_ident();
            if (argc >= 16)
                die("too many function parameters (max 16)");
            strncpy(params[argc], curtok.ident, 63);
            params[argc][63] = '\0';

            param_ptr_depth[argc] = ptrd;
            param_struct_id[argc] = this_struct_id;

            next_token();
            argc++;

            if (accept_sym(')'))
                break;
            expect_sym(',');
        }
    }

    if (accept_sym(';'))
    {
        int fi = func_find(fname);
        if (fi >= 0)
        {
            if (funcs[fi].offset != (uint32_t)0xFFFFFFFF)
                die("redefinition of function");
            funcs[fi].argc = argc;
            funcs[fi].is_variadic = is_variadic;
            funcs[fi].ret_ptr_depth = ret_ptr_depth;
            funcs[fi].ret_struct_id = ret_struct_id;
            for (int i = 0; i < argc; ++i)
            {
                funcs[fi].param_ptr_depth[i] = param_ptr_depth[i];
                funcs[fi].param_struct_id[i] = param_struct_id[i];
            }
        }
        else
        {
            if (funcs_cnt >= MAX_FUNCS)
                die("too many functions");
            strncpy(funcs[funcs_cnt].name, fname, sizeof(funcs[funcs_cnt].name) - 1);
            funcs[funcs_cnt].name[sizeof(funcs[funcs_cnt].name) - 1] = '\0';
            funcs[funcs_cnt].offset = (uint32_t)0xFFFFFFFF;
            funcs[funcs_cnt].argc = argc;
            funcs[funcs_cnt].is_variadic = is_variadic;
            funcs[funcs_cnt].ret_ptr_depth = ret_ptr_depth;
            funcs[funcs_cnt].ret_struct_id = ret_struct_id;
            for (int i = 0; i < argc; ++i)
            {
                funcs[funcs_cnt].param_ptr_depth[i] = param_ptr_depth[i];
                funcs[funcs_cnt].param_struct_id[i] = param_struct_id[i];
            }
            funcs_cnt++;
        }
        return;
    }

    uint32_t func_offset = (uint32_t)code_len;
    int existing = func_find(fname);
    if (existing >= 0)
    {
        if (funcs[existing].offset != (uint32_t)0xFFFFFFFF)
            die("redefinition of function");
        funcs[existing].offset = func_offset;
        funcs[existing].argc = argc;
        funcs[existing].is_variadic = is_variadic;
        funcs[existing].ret_ptr_depth = ret_ptr_depth;
        funcs[existing].ret_struct_id = ret_struct_id;
        for (int i = 0; i < argc; ++i)
        {
            funcs[existing].param_ptr_depth[i] = param_ptr_depth[i];
            funcs[existing].param_struct_id[i] = param_struct_id[i];
        }
    }
    else
    {
        if (funcs_cnt >= MAX_FUNCS)
            die("too many functions");
        strncpy(funcs[funcs_cnt].name, fname, sizeof(funcs[funcs_cnt].name) - 1);
        funcs[funcs_cnt].name[sizeof(funcs[funcs_cnt].name) - 1] = '\0';
        funcs[funcs_cnt].offset = func_offset;
        funcs[funcs_cnt].argc = argc;
        funcs[funcs_cnt].is_variadic = is_variadic;
        funcs[funcs_cnt].ret_ptr_depth = ret_ptr_depth;
        funcs[funcs_cnt].ret_struct_id = ret_struct_id;
        for (int i = 0; i < argc; ++i)
        {
            funcs[funcs_cnt].param_ptr_depth[i] = param_ptr_depth[i];
            funcs[funcs_cnt].param_struct_id[i] = param_struct_id[i];
        }
        funcs_cnt++;
    }

    emit_u8(0x55);
    emit_u8(0x89);
    emit_u8(0xE5);

    locals_cnt = 0;
    local_stack_size = 0;
    labels_cnt = 0;
    current_fun_argbytes = argc * 4;
    current_fun_saw_return = 0;
    if (strcmp(fname, "main") == 0 && current_fun_argbytes == 0)
        current_fun_argbytes = 8;
    for (int i = 0; i < argc; ++i)
    {
        int li = begin_local_slot(params[i], 0);
        locals[li].ptr_depth = param_ptr_depth[i];
        locals[li].struct_id = param_struct_id[i];

        int elem_size = 4;
        if (locals[li].struct_id >= 0 && locals[li].ptr_depth == 0)
            elem_size = structsyms[locals[li].struct_id].size;

        assign_local_frame(li, elem_size);
    }

    if (local_stack_size > 0)
        emit_sub_esp_imm((uint32_t)local_stack_size);

    for (int i = 0; i < argc; ++i)
    {
        int li = i;
        int param_src_disp = 8 + (argc - 1 - i) * 4;
        int elem_size = 4;
        if (locals[li].struct_id >= 0 && locals[li].ptr_depth == 0)
            elem_size = structsyms[locals[li].struct_id].size;
        if (elem_size < 4)
            elem_size = 4;

        if (elem_size == 4)
        {
            emit_mov_eax_from_ebp_disp_any((int32_t)param_src_disp);
            emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
        }
        else
        {
            for (int off = 0; off < elem_size; off += 4)
            {
                int src_disp = param_src_disp + off;
                emit_mov_eax_from_ebp_disp_any((int32_t)src_disp);
                emit_mov_ebp_disp_from_eax_any(locals[li].ebp_disp + off);
            }
        }
    }

    expect_sym('{');
    while (!accept_sym('}'))
        gen_stmt();

    if (!current_fun_saw_return)
    {
        emit_u8(0xB8);
        emit_u32(0);
        emit_leave();
        emit_ret_pop_args_uint16((uint16_t)current_fun_argbytes);
    }
}


void gen_global_decl(void)
{
    int is_extern = 0;
    int is_static = 0;
    int is_const = 0;

    if (accept_ident("extern"))
        is_extern = 1;
    if (accept_ident("static"))
        is_static = 1;
    while (accept_ident("volatile"))
        ;
    if (accept_ident("const"))
        is_const = 1;

    TypeSpec ts;
    if (!parse_type_spec(&ts))
        die("expected type in global declaration");

    if (ts.base == TY_VOID && ts.ptr_depth == 0)
        die("void global not allowed");

    expect_ident();
    char gname[64];
    strncpy(gname, curtok.ident, sizeof(gname) - 1);
    gname[sizeof(gname) - 1] = '\0';
    next_token();

    if (accept_sym('('))
    {
        int argc = 0;
        int is_variadic = 0;
        if (!accept_sym(')'))
        {
            while (1)
            {
                if (curtok.type == TOK_SYM && curtok.sym == '.' && src[0] == '.' && src[1] == '.')
                {
                    src += 2;
                    next_token();
                    is_variadic = 1;
                    break;
                }
                TypeSpec pts;
                if (!parse_type_spec(&pts))
                    die("expected parameter type");
                expect_ident();
                next_token();
                argc++;
                if (accept_sym(')'))
                    break;
                expect_sym(',');
            }
        }
        expect_sym(';');

        int fi = func_find(gname);
        if (fi < 0)
        {
            if (funcs_cnt >= MAX_FUNCS)
                die("too many functions");
            fi = funcs_cnt++;
            strncpy(funcs[fi].name, gname, sizeof(funcs[fi].name) - 1);
            funcs[fi].offset = 0xFFFFFFFFu;
            funcs[fi].argc = argc;
            funcs[fi].is_variadic = is_variadic;
            funcs[fi].is_extern = 1;
            {
                uint32_t dummy = 0;
                funcs[fi].is_libc = baoc_libc_sym_find(gname, &dummy) == 0 ? 1 : 0;
            }
        }
        else
        {
            funcs[fi].is_extern = 1;
            if (funcs[fi].offset == 0xFFFFFFFFu)
                {
                uint32_t dummy = 0;
                funcs[fi].is_libc = baoc_libc_sym_find(gname, &dummy) == 0 ? 1 : 0;
            }
        }
        return;
    }

    int has_init = 0;
    int32_t init_val = 0;
    if (accept_sym('='))
    {
        if (curtok.type != TOK_NUMBER)
            die("global init must be integer constant");
        init_val = curtok.val;
        has_init = 1;
        next_token();
    }

    expect_sym(';');
    add_global(gname, &ts, is_static, is_extern, is_const, has_init, init_val);
}


static void parse_struct_field(int si)
{
    while (curtok.type == TOK_IDENT &&
           (strcmp(curtok.ident, "const") == 0 || strcmp(curtok.ident, "volatile") == 0 ||
            strcmp(curtok.ident, "signed") == 0 || strcmp(curtok.ident, "unsigned") == 0))
        next_token();

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

void compile(const char *input)
{
    src = (char *)input;
    next_token();

    while (curtok.type != TOK_EOF)
    {
        if (curtok.type == TOK_IDENT && (strcmp(curtok.ident, "extern") == 0 || strcmp(curtok.ident, "static") == 0))
        {
            gen_global_decl();
            continue;
        }

        if (curtok.type == TOK_IDENT && is_type_name(curtok.ident))
        {
            Token save = curtok;
            char *save_src = src;
            next_token();
            while (accept_sym('*'))
                ;
            if (curtok.type == TOK_IDENT)
            {
                char *p = src;
                while (*p && isspace((unsigned char)*p))
                    p++;
                if (*p == '(')
                {
                    curtok = save;
                    src = save_src;
                    gen_function();
                    continue;
                }
                if (*p == ';' || *p == '=')
                {
                    curtok = save;
                    src = save_src;
                    gen_global_decl();
                    continue;
                }
            }
            curtok = save;
            src = save_src;
        }

        if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "typedef") == 0)
        {
            next_token();

            int base_struct_id = -1;
            int base_ptr_depth = 0;

            if (curtok.type != TOK_IDENT)
                die("expected type name after typedef");

            if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
            {
                int is_union = (strcmp(curtok.ident, "union") == 0);
                next_token();

                if (accept_sym('{'))
                {
                    char tmpn[64];
                    snprintf(tmpn, sizeof(tmpn), "__anon_struct_%d", structsyms_cnt);
                    int si = add_structsym(tmpn, is_union);

                    while (!accept_sym('}'))
                        parse_struct_field(si);

                    while (accept_sym('*'))
                        base_ptr_depth++;

                    expect_ident();
                    char tname[64];
                    strncpy(tname, curtok.ident, sizeof(tname) - 1);
                    tname[sizeof(tname) - 1] = '\0';
                    next_token();
                    expect_sym(';');
                    add_typedef_name(tname, base_ptr_depth, si);
                    continue;
                }

                expect_ident();
                base_struct_id = struct_find(curtok.ident);
                if (base_struct_id < 0)
                    die("unknown struct/union type in typedef");
                next_token();
            }
            else
            {
                if (strcmp(curtok.ident, "unsigned") == 0 || strcmp(curtok.ident, "signed") == 0)
                {
                    next_token();
                    if (curtok.type == TOK_IDENT && is_type_name(curtok.ident))
                        next_token();
                }

                if (curtok.type != TOK_IDENT)
                    die("expected type name after typedef");

                int td_ptr = 0, td_struct = -1;
                if (lookup_typedef(curtok.ident, &td_ptr, &td_struct))
                {
                    base_ptr_depth += td_ptr;
                    base_struct_id = td_struct;
                    next_token();
                }
                else
                {
                    next_token();
                }
            }

            while (accept_sym('*'))
                base_ptr_depth++;

            expect_ident();
            char tname[64];
            strncpy(tname, curtok.ident, sizeof(tname) - 1);
            tname[sizeof(tname) - 1] = '\0';
            next_token();

            expect_sym(';');

            add_typedef_name(tname, base_ptr_depth, base_struct_id);
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
                    parse_struct_field(si);

                if (sname[0] != '\0')
                    add_typedef_name(sname, 0, si);

                if (curtok.type == TOK_IDENT)
                {
                    char tname[64];
                    strncpy(tname, curtok.ident, 63);
                    tname[63] = '\0';
                    next_token();
                    expect_sym(';');

                    int existing = struct_find(tname);
                    int alias_struct_id = -1;
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
                        alias_struct_id = newidx;
                    }
                    else
                    {
                        alias_struct_id = existing;
                    }

                    add_typedef_name(tname, 0, alias_struct_id);
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
