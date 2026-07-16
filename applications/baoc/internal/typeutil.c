#include "baoc_internal.h"

void type_spec_init(TypeSpec *ts)
{
    memset(ts, 0, sizeof(*ts));
    ts->base = TY_INT;
    ts->size = 4;
}

int parse_type_spec(TypeSpec *ts)
{
    type_spec_init(ts);

    if (curtok.type != TOK_IDENT)
        return 0;

    while (curtok.type == TOK_IDENT &&
           (strcmp(curtok.ident, "const") == 0 || strcmp(curtok.ident, "volatile") == 0))
    {
        if (strcmp(curtok.ident, "const") == 0)
            ts->is_const = 1;
        next_token();
    }

    if (curtok.type != TOK_IDENT)
        return 0;

    {
        int si = struct_find(curtok.ident);
        if (si >= 0)
        {
            ts->base = TY_STRUCT;
            ts->struct_id = si;
            ts->size = structsyms[si].size;
            if (ts->size <= 0)
                ts->size = 4;
            next_token();
            while (accept_sym('*'))
            {
                ts->ptr_depth++;
                ts->size = 4;
            }
            return 1;
        }
    }

    if (strcmp(curtok.ident, "unsigned") == 0)
    {
        ts->unsigned_flag = 1;
        next_token();
        if (curtok.type == TOK_IDENT)
        {
            if (strcmp(curtok.ident, "char") == 0)
            {
                ts->base = TY_CHAR;
                ts->size = 1;
                next_token();
            }
            else if (strcmp(curtok.ident, "short") == 0)
            {
                ts->base = TY_SHORT;
                ts->size = 2;
                next_token();
            }
            else if (strcmp(curtok.ident, "int") == 0 || strcmp(curtok.ident, "long") == 0)
            {
                ts->base = TY_INT;
                ts->size = 4;
                next_token();
            }
            else
                return 0;
        }
        else
        {
            ts->base = TY_INT;
            ts->size = 4;
        }
    }
    else if (strcmp(curtok.ident, "signed") == 0)
    {
        next_token();
        if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "char") == 0)
        {
            ts->base = TY_CHAR;
            ts->size = 1;
            next_token();
        }
        else if (curtok.type == TOK_IDENT && strcmp(curtok.ident, "int") == 0)
        {
            ts->base = TY_INT;
            ts->size = 4;
            next_token();
        }
        else
        {
            ts->base = TY_INT;
            ts->size = 4;
        }
    }
    else if (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0)
    {
        next_token();
        expect_ident();
        ts->base = TY_STRUCT;
        ts->struct_id = struct_find(curtok.ident);
        if (ts->struct_id < 0)
            die("unknown struct/union type");
        ts->size = structsyms[ts->struct_id].size;
        if (ts->size <= 0)
            ts->size = 4;
        next_token();
    }
    else if (strcmp(curtok.ident, "void") == 0)
    {
        ts->base = TY_VOID;
        ts->size = 0;
        next_token();
    }
    else if (strcmp(curtok.ident, "char") == 0)
    {
        ts->base = TY_CHAR;
        ts->size = 1;
        next_token();
    }
    else if (strcmp(curtok.ident, "short") == 0)
    {
        ts->base = TY_SHORT;
        ts->size = 2;
        next_token();
    }
    else if (strcmp(curtok.ident, "long") == 0)
    {
        ts->base = TY_LONG;
        ts->size = 4;
        next_token();
    }
    else if (strcmp(curtok.ident, "float") == 0)
    {
        ts->base = TY_FLOAT;
        ts->size = 4;
        next_token();
    }
    else if (strcmp(curtok.ident, "double") == 0)
    {
        ts->base = TY_DOUBLE;
        ts->size = 8;
        next_token();
    }
    else if (strcmp(curtok.ident, "int") == 0)
    {
        ts->base = TY_INT;
        ts->size = 4;
        next_token();
    }
    else if (is_typedef_name(curtok.ident) || lookup_typedef(curtok.ident, &ts->ptr_depth, &ts->struct_id))
    {
        int td_ptr = 0, td_struct = -1;
        lookup_typedef(curtok.ident, &td_ptr, &td_struct);
        next_token();
        if (td_struct >= 0)
        {
            ts->base = TY_STRUCT;
            ts->struct_id = td_struct;
            ts->size = structsyms[td_struct].size;
            if (ts->size <= 0)
                ts->size = 4;
        }
        ts->ptr_depth = td_ptr;
    }
    else
        return 0;

    while (accept_sym('*'))
    {
        ts->ptr_depth++;
        ts->size = 4;
        ts->base = TY_INT;
    }

    if (ts->ptr_depth > 0)
        ts->size = 4;

    return 1;
}

int type_spec_size(const TypeSpec *ts)
{
    if (!ts)
        return 4;
    if (ts->ptr_depth > 0)
        return 4;
    if (ts->base == TY_STRUCT && ts->struct_id >= 0)
        return structsyms[ts->struct_id].size > 0 ? structsyms[ts->struct_id].size : 4;
    if (ts->size > 0)
        return ts->size;
    return 4;
}

int type_spec_is_float(const TypeSpec *ts)
{
    return ts && (ts->base == TY_FLOAT || ts->base == TY_DOUBLE);
}

int sizeof_type_name(void)
{
    if (!accept_sym('('))
        die("expected '(' after sizeof");

    if (curtok.type == TOK_IDENT && (strcmp(curtok.ident, "struct") == 0 || strcmp(curtok.ident, "union") == 0))
    {
        next_token();
        expect_ident();
        int si = struct_find(curtok.ident);
        if (si < 0)
            die("unknown struct in sizeof");
        next_token();
        expect_sym(')');
        return structsyms[si].size > 0 ? structsyms[si].size : 4;
    }

    if (curtok.type == TOK_IDENT && is_type_name(curtok.ident))
    {
        TypeSpec ts;
        parse_type_spec(&ts);
        expect_sym(')');
        return type_spec_size(&ts);
    }

    gen_expr();
    expect_sym(')');
    emit_pop_eax();
    return 4;
}

void local_from_typespec(int li, const TypeSpec *ts)
{
    locals[li].ptr_depth = ts->ptr_depth;
    locals[li].struct_id = ts->struct_id;
    locals[li].elem_size = type_spec_size(ts);
    locals[li].type_base = ts->base;
}
