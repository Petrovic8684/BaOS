#include "baoc_internal.h"

void gen_block()
{
    expect_sym('{');
    while (!accept_sym('}'))
        gen_stmt();
}


size_t curtok_text_len(const Token *t)
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


size_t resolve_jump_target(size_t tgt)
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

        int base_struct_id = -1;
        int base_ptr_depth = 0;

        if (curtok.type != TOK_IDENT)
            die("expected type name after typedef");

        if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
        {
            int is_union = (strcmp(curtok.ident, "union") == 0);
            next_token();
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
                if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
                    next_token();
            }
            else
            {
                int td_ptr = 0, td_struct = -1;
                if (!is_type_name(curtok.ident))
                    die("unsupported typedef base type");
                if (lookup_typedef(curtok.ident, &td_ptr, &td_struct))
                {
                    base_ptr_depth += td_ptr;
                    base_struct_id = td_struct;
                }
                next_token();
            }
        }

        while (accept_sym('*'))
            base_ptr_depth++;

        expect_ident();
        char tname[64];
        strncpy(tname, curtok.ident, 63);
        tname[63] = '\0';
        next_token();

        expect_sym(';');

        add_typedef_name(tname, base_ptr_depth, base_struct_id);
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
            current_fun_saw_return = 1;
            return;
        }
        else
        {
            gen_expr();
            emit_pop_eax();
            expect_sym(';');
            emit_leave();
            emit_ret_pop_args_uint16((uint16_t)current_fun_argbytes);
            current_fun_saw_return = 1;
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

    if (curtok.type == TOK_IDENT)
    {
        char *p = src;
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == ':')
        {
            char lname[64];
            strncpy(lname, curtok.ident, sizeof(lname) - 1);
            lname[sizeof(lname) - 1] = '\0';
            next_token();
            expect_sym(':');
            label_define(lname);
            gen_stmt();
            return;
        }
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "switch") == 0)
    {
        next_token();
        expect_sym('(');
        gen_expr();
        expect_sym(')');
        emit_pop_eax();
        emit_push_eax();

        if (switch_depth >= MAX_SWITCH_DEPTH)
            die("switch depth too big");
        switch_break_counts[switch_depth] = 0;
        switch_depth++;

        expect_sym('{');

        size_t default_start = 0;
        int has_default = 0;

        while (!accept_sym('}'))
        {
            if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "case") == 0)
            {
                next_token();
                if (curtok.type != TOK_NUMBER)
                    die("expected case constant");
                int cv = curtok.val;
                next_token();
                expect_sym(':');

                emit_pop_eax();
                emit_push_eax();
                emit_u8(0x3D);
                emit_u32((uint32_t)cv);
                size_t jne = emit_jnz_rel32_placeholder();
                emit_pop_eax();

                while (curtok.type != TOK_EOF &&
                       !(curtok.type == TOK_IDENT &&
                         (strcmp(curtok.ident, "case") == 0 || strcmp(curtok.ident, "default") == 0)) &&
                       !(curtok.type == TOK_SYM && curtok.sym == '}'))
                    gen_stmt();

                size_t jb = emit_jmp_rel32_placeholder();
                if (switch_break_counts[switch_depth - 1] < 64)
                    switch_break_addrs[switch_depth - 1][switch_break_counts[switch_depth - 1]++] = jb;
                patch_rel32_at(jne, (uint32_t)(code_len - (jne + 4)));
            }
            else if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "default") == 0)
            {
                next_token();
                expect_sym(':');
                has_default = 1;
                default_start = code_len;
                while (curtok.type != TOK_EOF &&
                       !(curtok.type == TOK_IDENT && strcmp(curtok.ident, "case") == 0) &&
                       !(curtok.type == TOK_SYM && curtok.sym == '}'))
                    gen_stmt();
                size_t jb = emit_jmp_rel32_placeholder();
                if (switch_break_counts[switch_depth - 1] < 64)
                    switch_break_addrs[switch_depth - 1][switch_break_counts[switch_depth - 1]++] = jb;
            }
            else
                gen_stmt();
        }

        size_t end_sw = code_len;
        emit_pop_eax();

        for (int i = 0; i < switch_break_counts[switch_depth - 1]; i++)
            patch_rel32_at(switch_break_addrs[switch_depth - 1][i],
                           (uint32_t)(end_sw - (switch_break_addrs[switch_depth - 1][i] + 4)));

        (void)default_start;
        (void)has_default;
        switch_depth--;
        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "goto") == 0)
    {
        next_token();
        expect_ident();
        char lname[64];
        strncpy(lname, curtok.ident, sizeof(lname) - 1);
        lname[sizeof(lname) - 1] = '\0';
        next_token();
        expect_sym(';');
        int li = label_find(lname);
        if (li < 0)
            die("unknown label");
        size_t target = label_get_addr(li);
        if (target == 0)
            die("forward goto not supported");
        size_t jmp = emit_jmp_rel32_placeholder();
        patch_rel32_at(jmp, (uint32_t)(target - (jmp + 4)));
        return;
    }

    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "break") == 0)
    {
        next_token();
        expect_sym(';');
        if (switch_depth > 0)
        {
            size_t off = emit_jmp_rel32_placeholder();
            if (switch_break_counts[switch_depth - 1] >= 64)
                die("too many break in switch");
            switch_break_addrs[switch_depth - 1][switch_break_counts[switch_depth - 1]++] = off;
            return;
        }
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

    if (curtok.type == TOK_IDENT && (strcmp(curtok.ident, "const") == 0 || is_type_name(curtok.ident) || strcmp(curtok.ident, "static") == 0 || strcmp(curtok.ident, "auto") == 0 || strcmp(curtok.ident, "volatile") == 0))
    {
        int is_const = 0;
        int is_static_local = 0;
        if (strcmp(curtok.ident, "static") == 0)
        {
            is_static_local = 1;
            next_token();
        }
        if (strcmp(curtok.ident, "auto") == 0 || strcmp(curtok.ident, "volatile") == 0)
            next_token();

        if (strcmp(curtok.ident, "const") == 0)
        {
            is_const = 1;
            next_token();
            if (!(curtok.type == TOK_IDENT && is_type_name(curtok.ident)))
                die("expected type after 'const'");
        }

        TypeSpec ts;
        if (!parse_type_spec(&ts))
            die("expected type in declaration");
        if (type_spec_is_float(&ts))
            die("float operations not supported");

        expect_ident();
        char vname[64];
        strncpy(vname, curtok.ident, 63);
        vname[63] = '\0';
        next_token();

        if (is_static_local)
        {
            char mname[80];
            snprintf(mname, sizeof(mname), "__%s_%s", current_func_name, vname);
            int has_init = 0;
            int32_t init_val = 0;
            if (accept_sym('='))
            {
                if (curtok.type != TOK_NUMBER)
                    die("static local init must be constant");
                init_val = curtok.val;
                has_init = 1;
                next_token();
            }
            add_global(mname, &ts, 1, 0, is_const, has_init, init_val);
            expect_sym(';');
            return;
        }

        int li = begin_local_slot(vname, is_const);
        local_from_typespec(li, &ts);

        int arrsz = 0;
        int elem_size = locals[li].elem_size;

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
            locals[li].arr_size = arrsz;
            finish_local_alloc(li, total);
        }
        else
            finish_local_alloc(li, elem_size);

        if (accept_sym('='))
        {
            if (locals[li].arr_size > 0 && accept_sym('{'))
            {
                int idx = 0;
                int elem = locals[li].elem_size > 0 ? locals[li].elem_size : 4;
                while (1)
                {
                    gen_expr();
                    emit_pop_eax();
                    emit_mov_ebp_disp_from_eax_any(local_addr_disp(li) + idx * elem);
                    idx++;
                    if (accept_sym('}'))
                        break;
                    expect_sym(',');
                }
            }
            else
            {
                gen_expr();
                emit_pop_eax();
                emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
            }
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

        next_token();

        if (curtok.type == TOK_SYM && curtok.sym == '=')
        {
            curtok = save;
            src = save_src;
            gen_assign();
            emit_pop_eax();
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
