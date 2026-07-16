#include "baoc_internal.h"

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
    emit_u8(0x36);
    emit_u8(0x8B);
    emit_u8(0x00);
}


void emit_load_eax_from_ebx_ptr()
{
    emit_u8(0x36);
    emit_u8(0x8B);
    emit_u8(0x03);
}


void emit_mov_ebx_from_eax()
{
    emit_u8(0x89);
    emit_u8(0xC3);
}


void emit_mov_ptr_ebx_from_eax()
{
    emit_u8(0x36);
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
