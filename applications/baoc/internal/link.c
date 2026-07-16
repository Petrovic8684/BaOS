#include "baoc_internal.h"
#include <errno.h>

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

static uint32_t resolve_func_vaddr(const char *name, uint32_t *out_is_user)
{
    int fi = func_find(name);
    if (fi >= 0 && funcs[fi].offset != 0xFFFFFFFFu)
    {
        if (out_is_user)
            *out_is_user = 1;
        return link_layout.user_text_vaddr + funcs[fi].offset;
    }

    uint32_t loff = 0;
    if (baoc_libc_sym_find(name, &loff) == 0)
    {
        if (out_is_user)
            *out_is_user = 0;
        return LOAD_BASE + link_layout.crt_size + loff;
    }

    return 0;
}

int baoc_link_elf(const char *out_path)
{
    uint32_t user_code_size = (uint32_t)code_len;

    uint32_t total_str_size = 0;
    for (int i = 0; i < strlits_cnt; ++i)
        total_str_size += (uint32_t)(strlen(strlits[i].s) + 1);

    link_layout.crt_size = (uint32_t)crt_sz;
    link_layout.libc_size = (uint32_t)libc_sz;
    link_layout.user_text_off = link_layout.crt_size + link_layout.libc_size;
    link_layout.user_text_vaddr = LOAD_BASE + link_layout.user_text_off;
    link_layout.rodata_off = link_layout.user_text_off + user_code_size;
    link_layout.rodata_vaddr = LOAD_BASE + link_layout.rodata_off;
    link_layout.data_off = link_layout.rodata_off + total_str_size;
    link_layout.data_vaddr = LOAD_BASE + link_layout.data_off;
    link_layout.bss_off = link_layout.data_off + data_size;
    link_layout.bss_vaddr = LOAD_BASE + link_layout.bss_off;
    link_layout.total_filesz = link_layout.bss_off + bss_size;
    link_layout.total_memsz = link_layout.total_filesz;

    uint32_t ph_offset = sizeof(Elf32_Ehdr);
    uint32_t seg_file_off = ph_offset + sizeof(Elf32_Phdr);
    uint32_t elf_size = seg_file_off + link_layout.total_filesz;

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
    ph.p_offset = seg_file_off;
    ph.p_vaddr = LOAD_BASE;
    ph.p_paddr = LOAD_BASE;
    ph.p_filesz = link_layout.total_filesz;
    ph.p_memsz = link_layout.total_memsz;
    ph.p_flags = 7;
    ph.p_align = 0x1000;

    memcpy(image_buf + ph_offset, &ph, sizeof(ph));

    memcpy(image_buf + seg_file_off, crt_buf, crt_sz);
    memcpy(image_buf + seg_file_off + link_layout.crt_size, libc_buf, libc_sz);
    memcpy(image_buf + seg_file_off + link_layout.user_text_off, code_buf, user_code_size);

    uint32_t cur_file_off = seg_file_off + link_layout.rodata_off;
    uint32_t cur_vaddr = link_layout.rodata_vaddr;
    for (int i = 0; i < strlits_cnt; ++i)
    {
        size_t len = strlen(strlits[i].s) + 1;
        memcpy(image_buf + cur_file_off, strlits[i].s, len);

        uint32_t imm_file_loc = seg_file_off + link_layout.user_text_off + strlits[i].code_place;
        uint32_t code_opcode_file = imm_file_loc - 1;
        if (strlits[i].code_place == 0 || image_buf[code_opcode_file] != 0x68)
            die("expected push imm at recorded place");

        uint32_t target_vaddr = cur_vaddr;
        memcpy(image_buf + imm_file_loc, &target_vaddr, 4);

        cur_file_off += (uint32_t)len;
        cur_vaddr += (uint32_t)len;
    }

    for (int g = 0; g < globals_cnt; g++)
    {
        if (globals[g].is_extern || globals[g].data_offset == 0xFFFFFFFFu)
            continue;
        if (!globals[g].has_init)
            continue;
        uint32_t off = seg_file_off + link_layout.data_off + globals[g].data_offset;
        memcpy(image_buf + off, &globals[g].init_val, 4);
    }

    for (int r = 0; r < relocs_cnt; r++)
    {
        uint32_t is_user = 0;
        uint32_t target = resolve_func_vaddr(relocs[r].sym_name[0] ? relocs[r].sym_name : relocs[r].name,
                                             &is_user);
        if (target == 0)
        {
            fprintf(stderr,
                    "\033[31mError: call to unknown function '%s' (missing /lib/baoc_syms or extra .c file?).\033[0m\n",
                    relocs[r].sym_name[0] ? relocs[r].sym_name : relocs[r].name);
            die("call to unknown function (final patch)");
        }

        uint32_t call_site = relocs[r].call_site;
        if (call_site == 0)
            die("invalid call site");

        uint32_t imm_file_loc = seg_file_off + link_layout.user_text_off + call_site;
        uint32_t call_instr_virt = link_layout.user_text_vaddr + (call_site - 1u);

        int32_t rel32 = (int32_t)(target - (call_instr_virt + 5u));
        memcpy(image_buf + imm_file_loc, &rel32, 4);
    }

    int call_offset_in_crt = -1;
    for (int i = 0; i + 4 < (int)crt_sz; ++i)
        if (crt_buf[i] == 0xE8)
        {
            call_offset_in_crt = i;
            break;
        }

    if (call_offset_in_crt >= 0)
    {
        int mi = func_find("main");
        if (mi < 0 || funcs[mi].offset == 0xFFFFFFFFu)
            die("main not defined");

        uint32_t call_place_in_file = seg_file_off + (uint32_t)call_offset_in_crt;
        uint32_t call_instr_virt = LOAD_BASE + (uint32_t)call_offset_in_crt;
        uint32_t main_target_virt = link_layout.user_text_vaddr + funcs[mi].offset;

        int32_t rel32 = (int32_t)(main_target_virt - (call_instr_virt + 5u));
        memcpy(image_buf + call_place_in_file + 1, &rel32, 4);
    }
    else
        fprintf(stderr, "\033[1;33mWarning: no CALL opcode found in crt0.\033[0m\n");

    if (file_write_all(out_path, image_buf, (size_t)elf_size) != 0)
    {
        fprintf(stderr, "\033[31mError: cannot write output '%s' (%s).\033[0m\n",
                out_path, strerror(errno));
        return -1;
    }

    return (int)elf_size;
}
