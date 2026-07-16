#include "baoc_internal.h"

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
            {
                int gi = global_find(save.ident);
                if (gi < 0)
                    gi = static_local_global_index(save.ident);
                if (gi >= 0)
                {
                    next_token();
                    gen_expr();
                    emit_pop_eax();
                    emit_store_global_from_eax(gi);
                    emit_push_eax();
                    return;
                }
                die("unknown variable on assign");
            }
            if (locals[li].is_const)
                die("assignment to const variable");
            emit_pop_eax();
            emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
            emit_push_eax();
            return;
        }

        if (curtok.type == TOK_SYM && curtok.sym == '[')
        {
            int li = local_find(save.ident);
            if (li < 0)
            {
                int gi = global_find(save.ident);
                if (gi < 0)
                    gi = static_local_global_index(save.ident);
                if (gi >= 0)
                {
                    next_token();
                    gen_expr();
                    emit_pop_eax();
                    emit_store_global_from_eax(gi);
                    emit_push_eax();
                    return;
                }
                die("unknown variable on assign");
            }
            if (locals[li].is_const)
                die("assignment to const variable");
            if (locals[li].arr_size > 0)
                emit_lea_eax_ebp_disp_any(local_addr_disp(li));
            else
                emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
            emit_push_eax();

            while (curtok.type == TOK_SYM && curtok.sym == '[')
            {
                next_token();

                if (curtok.type == TOK_NUMBER)
                {
                    int idx = curtok.val;
                    next_token();
                    expect_sym(']');

                    if (curtok.type == TOK_SYM && curtok.sym == '=')
                    {
                        next_token();
                        gen_expr();
                        emit_pop_eax();
                        if (locals[li].arr_size > 0)
                            emit_mov_ebp_disp_from_eax_any(local_addr_disp(li) + idx * 4);
                        else
                        {
                            emit_push_eax();
                            emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
                            if (idx != 0)
                            {
                                emit_u8(0x05);
                                emit_u32((uint32_t)(idx * 4));
                            }
                            emit_mov_ebx_from_eax();
                            emit_pop_eax();
                            emit_mov_ptr_ebx_from_eax();
                        }
                        emit_push_eax();
                        return;
                    }

                    if (locals[li].arr_size > 0)
                    {
                        emit_mov_eax_from_ebp_disp_any(local_addr_disp(li) + idx * 4);
                        emit_push_eax();
                        last_primary_is_simple = 0;
                        return;
                    }

                    emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
                    if (idx != 0)
                    {
                        emit_u8(0x05);
                        emit_u32((uint32_t)(idx * 4));
                    }
                    emit_mov_ebx_from_eax();
                    emit_load_eax_from_ebx_ptr();
                    emit_push_eax();
                    last_primary_is_simple = 0;
                    return;
                }

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

            if ((curtok.type == TOK_SYM && curtok.sym == '.') ||
                (curtok.type == TOK_OP && curtok.op == OP_ARROW))
            {
                emit_pop_eax();
                curtok = save;
                src = save_src;
            }
            else
            {
                emit_pop_ebx();
                emit_load_eax_from_ebx_ptr();
                emit_push_eax();
                last_primary_is_simple = 0;
                return;
            }
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
                {
                    int gi = global_find(save.ident);
                    if (gi < 0)
                        gi = static_local_global_index(save.ident);
                    if (gi >= 0)
                    {
                        emit_mov_eax_from_abs(global_vaddr(gi));
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
                        emit_mov_abs_from_eax(global_vaddr(gi));
                        emit_push_eax();
                        return;
                    }
                    die("unknown variable on compound assign");
                }
                if (locals[li].is_const)
                    die("assignment to const variable");
                emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
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
                emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
                emit_push_eax();
                return;
            }
        }

        if ((curtok.type == TOK_SYM && curtok.sym == '.') ||
            (curtok.type == TOK_OP && curtok.op == OP_ARROW))
        {
            int li = local_find(save.ident);
            if (li < 0)
            {
                int gi = global_find(save.ident);
                if (gi < 0)
                    gi = static_local_global_index(save.ident);
                if (gi >= 0)
                {
                    next_token();
                    gen_expr();
                    emit_pop_eax();
                    emit_store_global_from_eax(gi);
                    emit_push_eax();
                    return;
                }
                die("unknown variable on assign");
            }
            if (locals[li].is_const)
                die("assignment to const variable");

            if (curtok.type == TOK_OP && curtok.op == OP_ARROW)
                emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
            else
                emit_lea_eax_ebp_disp_any(local_addr_disp(li));
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
            int sz = sizeof_type_name();
            emit_push_imm32((uint32_t)sz);
            last_primary_is_simple = 0;
            return;
        }

        if (accept_sym('('))
        {
            gen_call_args_cdecl();
            gen_emit_call_by_name(name);
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
                int gi = global_find(name);
                if (gi < 0)
                    gi = static_local_global_index(name);
                if (gi >= 0)
                {
                    emit_load_global_to_stack(gi);
                    last_primary_is_simple = 1;
                    strncpy(last_primary_ident, name, sizeof(last_primary_ident) - 1);
                    last_primary_ident[sizeof(last_primary_ident) - 1] = '\0';
                    return;
                }
                die("unknown variable");
            }

            int had_index = 0;

            if (locals[li].arr_size > 0)
            {
                emit_lea_eax_ebp_disp_any(local_addr_disp(li));
                emit_push_eax();
            }
            else
            {
                emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
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
                emit_mov_ebx_from_eax();
                emit_load_eax_from_ebx_ptr();
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
                    emit_lea_eax_ebp_disp_any(local_addr_disp(li));
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

                emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
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
    if (curtok.type == TOK_SYM && curtok.sym == '(' && !in_cast_expr)
    {
        char *save_src = src;
        Token save_tok = curtok;
        next_token();
        if (curtok.type == TOK_IDENT && is_type_name(curtok.ident))
        {
            TypeSpec ts;
            parse_type_spec(&ts);
            if (type_spec_is_float(&ts))
                die("float operations not supported");
            expect_sym(')');
            in_cast_expr = 1;
            gen_unary();
            in_cast_expr = 0;
            return;
        }
        src = save_src;
        curtok = save_tok;
    }

    if (curtok.type == TOK_OP && curtok.op == OP_COMPL)
    {
        next_token();
        gen_unary();
        emit_pop_eax();
        emit_u8(0xF7);
        emit_u8(0xD0);
        emit_push_eax();
        return;
    }

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
            char aname[64];
            strncpy(aname, curtok.ident, sizeof(aname) - 1);
            aname[sizeof(aname) - 1] = '\0';
            next_token();
            if (accept_sym('['))
            {
                int li = local_find(aname);
                if (li < 0)
                    die("address-of: unknown variable");
                gen_expr();
                expect_sym(']');
                emit_pop_ebx();
                if (locals[li].arr_size > 0)
                    emit_lea_eax_ebp_disp_any(local_addr_disp(li));
                else
                    emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
                emit_shl_ebx_imm(2);
                emit_add_eax_ebx();
                emit_push_eax();
                return;
            }
            int li = local_find(aname);
            if (li >= 0)
            {
                emit_lea_eax_ebp_disp_any(local_addr_disp(li));
                emit_push_eax();
                return;
            }
            int gi = global_find(aname);
            if (gi >= 0)
            {
                emit_lea_eax_abs(global_vaddr(gi));
                emit_push_eax();
                return;
            }
            die("address-of: unknown variable");
        }
        gen_unary();
        emit_pop_eax();
        emit_push_eax();
        return;
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

        emit_mov_eax_from_ebp_disp_any(local_addr_disp(li));
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
        emit_mov_ebp_disp_from_eax_any(local_addr_disp(li));
        emit_push_eax();
        next_token();
        return;
    }

    gen_primary();

    if (accept_sym('('))
    {
        gen_call_args_cdecl();
        gen_emit_call_eax_indirect();
        last_primary_is_simple = 0;
    }
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


void gen_expr()
{
    gen_assign();
    while (accept_sym(','))
    {
        emit_pop_eax();
        gen_assign();
        emit_push_eax();
    }
}
