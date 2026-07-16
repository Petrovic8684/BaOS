#!/usr/bin/env python3
"""Split monolithic baoc.c into modular compilation-phase source files."""

import os
import re
import sys

ROOT = os.path.join(os.path.dirname(__file__), "..", "applications", "baoc")
INTERNAL = os.path.join(ROOT, "internal")
MONOLITH = os.environ.get("BAOC_MONOLITH", "/tmp/baoc_monolith.c")

MODULE_MAP = {
    "common.c": {"die"},
    "preprocess.c": {
        "pp_macros_clear_all",
        "pp_macro_find",
        "pp_macro_alloc",
        "pp_macro_undef",
        "pp_macro_add_object",
        "pp_macro_add_function",
        "preclean_input_into",
        "read_file_into",
        "pp_is_ident_char",
        "parse_macro_args_into",
        "pp_macro_instantiate_to",
        "expand_macros_line_to",
        "trim_inplace",
        "eval_simple_if_expr_pp",
        "pp_find_include_file",
        "preprocess_src",
    },
    "symtab.c": {
        "struct_find",
        "add_structsym",
        "add_struct_field",
        "struct_field_offset",
        "struct_field_size",
        "is_typedef_name",
        "add_typedef_name",
        "lookup_typedef",
        "enum_find",
        "add_enum_const",
        "add_string_literal_entry",
        "is_type_name",
        "func_find",
        "add_reloc",
        "local_find",
        "alloc_local",
    },
    "lexer.c": {
        "next_token",
        "accept_ident",
        "accept_sym",
        "expect_sym",
        "expect_ident",
    },
    "codegen.c": {
        "emit_u8",
        "emit_u16",
        "emit_u32",
        "emit_push_imm32",
        "emit_pop_eax",
        "emit_pop_ebx",
        "emit_push_eax",
        "emit_mov_eax_from_ebp_disp_any",
        "emit_mov_ebp_disp_from_eax_any",
        "emit_lea_eax_ebp_disp_any",
        "emit_load_eax_from_eax_ptr",
        "emit_mov_ptr_ebx_from_eax",
        "emit_shl_ebx_imm",
        "emit_add_eax_ebx",
        "emit_sub_eax_ebx",
        "emit_imul_eax_ebx",
        "emit_cdq",
        "emit_idiv_ebx",
        "emit_test_eax_eax",
        "emit_jz_rel32_placeholder",
        "emit_jnz_rel32_placeholder",
        "emit_jmp_rel32_placeholder",
        "patch_rel32_at",
        "emit_call_rel32_placeholder",
        "emit_leave",
        "emit_ret_pop_args_uint16",
        "emit_sub_esp_imm",
        "emit_setcc_and_push",
    },
    "parse_expr.c": {
        "gen_assign",
        "gen_primary",
        "gen_unary",
        "gen_mul",
        "gen_add",
        "gen_shift",
        "gen_bitwise",
        "gen_logic_and",
        "gen_logic_or",
        "gen_ternary",
        "gen_cmp",
        "gen_expr",
    },
    "parse_stmt.c": {
        "gen_block",
        "curtok_text_len",
        "resolve_jump_target",
        "gen_stmt",
    },
    "parse_decl.c": {
        "gen_function",
        "compile",
    },
}

HEADER_INCLUDES = """\
#ifndef BAOC_INTERNAL_H
#define BAOC_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
"""

HEADER_TYPES_START = 8  # 1-based line after std includes in monolith
HEADER_TYPES_END = 187  # through pp_macros declaration

STATE_START = 17  # global variable definitions start
STATE_END = 187

MODULE_INCLUDE = '#include "baoc_internal.h"\n\n'


def find_functions(source):
    items = []
    i = 0
    n = len(source)

    while i < n:
        while i < n and source[i] in " \t\n":
            i += 1
        if i >= n:
            break

        if source[i:].startswith("#"):
            j = source.find("\n", i)
            if j == -1:
                break
            i = j + 1
            continue

        start = i
        depth = 0
        in_str = False
        in_chr = False
        esc = False
        proto = True
        brace_start = -1

        while i < n:
            c = source[i]
            if in_str:
                if esc:
                    esc = False
                elif c == "\\":
                    esc = True
                elif c == '"':
                    in_str = False
            elif in_chr:
                if esc:
                    esc = False
                elif c == "\\":
                    esc = True
                elif c == "'":
                    in_chr = False
            elif c == '"':
                in_str = True
            elif c == "'":
                in_chr = True
            elif c == "{":
                if proto:
                    brace_start = i
                    proto = False
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0 and brace_start >= 0:
                    i += 1
                    while i < n and source[i] in " \t\n":
                        i += 1
                    if i < n and source[i] == ";":
                        i += 1
                    items.append(source[start:i])
                    break
            elif c == ";" and proto and depth == 0:
                chunk = source[start : i + 1]
                if "=" in chunk or chunk.strip().startswith("static"):
                    items.append(chunk)
                i += 1
                break
            i += 1
        else:
            break

    return items


def func_name(chunk):
    m = re.search(
        r"(?:^|\n)\s*(?:static\s+)?(?:inline\s+)?(?:const\s+)?"
        r"(?:unsigned\s+|signed\s+)?"
        r"(?:struct\s+\w+\s*\*?\s+|void\s*\*?\s+|char\s*\*?\s+|int\s+|size_t\s+|long\s+|"
        r"PP_Macro\s*\*?\s+)?"
        r"(\w+)\s*\(",
        chunk,
    )
    return m.group(1) if m else None


def line_range(source, needle):
    idx = source.find(needle)
    if idx < 0:
        return None
    return source.count("\n", 0, idx) + 1


def extract_lines(lines, start, end):
    return "".join(lines[start - 1 : end])


def build_header(lines):
    body = extract_lines(lines, HEADER_TYPES_START, HEADER_TYPES_END)
    # Convert definitions to extern declarations for globals
    decls = []
    for line in body.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or stripped.startswith("typedef"):
            decls.append(line)
            continue
        if stripped.startswith("static"):
            continue
        if "=" in stripped and stripped.endswith(";"):
            name = stripped.split("=")[0].strip().split()[-1]
            if "[" in name:
                name = name.split("[")[0]
            base = stripped.split("=")[0].rstrip()
            decls.append(f"extern {base};")
        elif stripped.endswith(";") and not stripped.startswith("}"):
            if any(
                t in stripped
                for t in ("Reloc ", "FuncSym ", "LocalVar ", "TypedefSym ", "EnumConst ",
                          "StrLit ", "StructSym ", "Token ", "PP_Macro ", "char *src")
            ) or stripped.startswith("Token curtok"):
                decls.append("extern " + stripped if not stripped.startswith("extern") else stripped)
            else:
                decls.append(line)
        else:
            decls.append(line)

    prototypes = """
void die(const char *m);

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
int local_find(const char *name);
int alloc_local(const char *name, int is_const);

static inline int32_t neg_off(int offset) { return -(int32_t)offset; }

void gen_expr(void);
void gen_cmp(void);
void gen_ternary(void);
void gen_stmt(void);
void gen_function(void);
void compile(const char *input);

int baoc_link_elf(const char *out_path, long crt_sz);

#endif
"""
    return HEADER_INCLUDES + "\n" + "\n".join(decls) + prototypes


def build_state(lines):
    chunks = []
    for line in lines[STATE_START - 1 : STATE_END]:
        if line.strip().startswith("static"):
            chunks.append(line)
        elif "=" in line and line.strip().endswith(";"):
            chunks.append(line)
        elif line.strip().startswith("Token curtok") or line.strip().startswith("char *src"):
            chunks.append(line)
        elif any(
            x in line
            for x in (
                "Reloc relocs",
                "FuncSym funcs",
                "LocalVar locals",
                "TypedefSym typedefs",
                "EnumConst enum_consts",
                "StrLit strlits",
                "StructSym structsyms",
            )
        ):
            chunks.append(line)
        elif line.strip() and not line.strip().startswith("#") and not line.strip().startswith("typedef"):
            if line.strip().endswith(";") and "[" in line:
                chunks.append(line)

    return '#include "baoc_internal.h"\n\n' + "".join(chunks)


def normalize_chunk(chunk, public_names):
    name = func_name(chunk)
    if name and name in public_names:
        chunk = re.sub(r"\bstatic\s+", "", chunk, count=1)
    return chunk.strip() + "\n"


def extract_link_and_main(source, lines):
    elf_typedef = extract_lines(lines, 3566, 3577)
    main_body_start = 3579
    link_start_marker = "    uint32_t user_code_size = (uint32_t)code_len;"
    main_text = extract_lines(lines, main_body_start, len(lines))
    link_idx = main_text.find(link_start_marker)
    if link_idx < 0:
        raise SystemExit("could not find link section in main()")

    setup = main_text[:link_idx].rstrip()
    link_part = main_text[link_idx:]
    # Remove final file write/printf from link part; keep through reloc loops
    link_part = link_part.replace(
        "    FILE *out = fopen(argv[2], \"wb\");\n"
        "    if (!out)\n"
        "        die(\"cannot open output\");\n\n"
        "    fwrite(image_buf, 1, elf_size, out);\n"
        "    fclose(out);\n\n"
        "    printf(\"\\033[32mWrote %s (%u bytes) as ELF (simple subset compiler, no libc).\\033[0m\\n\", argv[2], elf_size);\n"
        "    return 0;\n",
        "    return (int)elf_size;\n",
    )

    link_fn = (
        '#include "baoc_internal.h"\n\n'
        + elf_typedef
        + "\n"
        + "int baoc_link_elf(const char *out_path, long crt_sz)\n{\n"
        + link_part
        + "}\n"
    )

    main_fn = (
        "#include \"baoc_internal.h\"\n\n"
        + "int main(int argc, char **argv)\n{\n"
        + setup.split("int main(int argc, char **argv)\n{", 1)[1]
        + "\n    int elf_size = baoc_link_elf(argv[2], crt_sz);\n"
        + "    if (elf_size < 0)\n"
        + "        return 1;\n"
        + "    printf(\"\\033[32mWrote %s (%u bytes) as ELF (simple subset compiler, no libc).\\033[0m\\n\", argv[2], (unsigned)elf_size);\n"
        + "    return 0;\n"
        + "}\n"
    )
    return link_fn, main_fn


def main():
    if not os.path.exists(MONOLITH):
        print(f"missing monolith: {MONOLITH}", file=sys.stderr)
        sys.exit(1)

    with open(MONOLITH, "r") as f:
        source = f.read()
    lines = source.splitlines(keepends=True)

    os.makedirs(INTERNAL, exist_ok=True)

    name_to_module = {}
    public_names = set()
    for module, names in MODULE_MAP.items():
        for name in names:
            name_to_module[name] = module
            public_names.add(name)
    public_names.add("preprocess_src")
    public_names.add("baoc_link_elf")

    with open(os.path.join(INTERNAL, "baoc_internal.h"), "w") as f:
        f.write(build_header(lines))

    with open(os.path.join(INTERNAL, "state.c"), "w") as f:
        f.write(build_state(lines))

    modules = {m: [] for m in MODULE_MAP}
    items = find_functions(source)
    assigned = set()

    for chunk in items:
        name = func_name(chunk)
        if not name:
            continue
        mod = name_to_module.get(name)
        if not mod:
            print(f"warning: unassigned function {name}", file=sys.stderr)
            continue
        modules[mod].append(normalize_chunk(chunk, public_names))
        assigned.add(name)

    for mod, chunks in modules.items():
        if not chunks:
            continue
        out_path = os.path.join(INTERNAL, mod)
        with open(out_path, "w") as f:
            f.write(MODULE_INCLUDE + "\n\n".join(chunks))

    link_fn, main_fn = extract_link_and_main(source, lines)
    with open(os.path.join(INTERNAL, "link.c"), "w") as f:
        f.write(link_fn)
    with open(os.path.join(ROOT, "main.c"), "w") as f:
        f.write(main_fn)

    print(f"split baoc -> {ROOT}/ ({len(assigned)} functions)")


if __name__ == "__main__":
    main()
