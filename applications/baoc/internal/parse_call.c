#include "baoc_internal.h"

void gen_call_args_cdecl(void)
{
    if (accept_sym(')'))
        return;

    gen_expr();
    while (accept_sym(','))
        gen_expr();
    expect_sym(')');
}

void gen_emit_call_by_name(const char *name)
{
    size_t call_site = emit_call_rel32_placeholder();
    add_reloc(name, (uint32_t)call_site);
}

void gen_emit_call_eax_indirect(void)
{
    emit_pop_eax();
    emit_u8(0xFF);
    emit_u8(0xD0);
    emit_push_eax();
}
