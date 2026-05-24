#!/usr/bin/env python3
"""Split monolithic runtime .c files into fine-grained modules."""

import os
import re
import sys

ROOT = os.path.join(os.path.dirname(__file__), "..", "runtime", "src")

MONOLITHS = {
    "ctype.c": "ctype",
    "string.c": "string",
    "math.c": "math",
    "errno.c": "errno",
    "libgen.c": "libgen",
    "sys_info.c": "sys",
    "sys_stat.c": "sys",
    "sys_utsname.c": "sys",
    "baos_mouse.c": "baos",
    "baos_vga.c": "baos",
    "baos_sound.c": "baos",
}


def find_functions(source):
    """Extract top-level C functions and file-scope static data blocks."""
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
        r"(?:unsigned\s+|signed\s+)?(?:long\s+long\s+|long\s+|short\s+)?"
        r"(?:struct\s+\w+\s*\*?\s+|void\s*\*?\s+|char\s*\*?\s+|int\s+|size_t\s+|"
        r"double\s+|float\s+|FILE\s*\*?\s+|DIR\s*\*?\s+|div_t\s+|ldiv_t\s+|lldiv_t\s+|"
        r"time_t\s+|struct\s+tm\s*\*?\s+|struct\s+dirent\s*\*?\s+)?"
        r"(\w+)\s*\(",
        chunk,
    )
    return m.group(1) if m else None


def includes_for(module, chunk, orig_includes):
    incs = set(orig_includes)
    if module in ("string", "stdlib", "stdio", "dirent", "unistd", "time", "libgen", "errno"):
        pass
    body = chunk
    if "malloc" in body or "free" in body or "realloc" in body or "calloc" in body:
        incs.add("#include <stdlib.h>")
    if "strlen" in body or "strcpy" in body or "memcpy" in body or "strcmp" in body:
        incs.add("#include <string.h>")
    if "errno" in body and module != "errno":
        incs.add("#include <errno.h>")
    if "isdigit" in body or "isspace" in body or "tolower" in body or "isalpha" in body:
        incs.add("#include <ctype.h>")
    if "printf" in body or "fprintf" in body or "fputc" in body or "FILE" in body:
        incs.add("#include <stdio.h>")
    if "pow(" in body or "log(" in body or "sqrt(" in body or "exp(" in body or "fabs(" in body:
        incs.add("#include <math.h>")
    if "va_list" in body or "va_start" in body:
        incs.add("#include <stdarg.h>")
    if "map_fs_error" in body:
        incs.add("#include <errno.h>")
    if "fs_where" in body or "fs_change_dir" in body:
        incs.add('"internal/fs_helpers.h"')
    if module == "math":
        incs.add('"internal/math_common.h"')
    if module == "stdio" and "tty_" in body:
        incs.add('"stdio/tty_internal.h"')
    if module == "stdio" and ("file_table" in body or "alloc_file" in body or "free_file" in body):
        incs.add('"stdio/file_internal.h"')
    if module == "dirent":
        incs.add('"dirent/dirent_internal.h"')
    if module == "time" and ("fill_tm" in body or "civil_from" in body or "time_from" in body):
        incs.add('"time/time_internal.h"')
    if module == "stdlib" and ("heap_" in body or "free_list" in body or "malloc" in chunk[:200]):
        incs.add('"stdlib/heap_internal.h"')
    return sorted(incs, key=lambda x: (x.startswith('"'), x))


def parse_includes(source):
    incs = []
    for line in source.splitlines():
        if line.startswith("#include"):
            incs.append(line.strip())
        elif line.strip() and not line.strip().startswith("//"):
            break
    return incs


def split_file(filename, module):
    src_path = os.path.join(ROOT, filename)
    if not os.path.exists(src_path):
        print(f"skip missing {filename}")
        return

    with open(src_path, "r") as f:
        source = f.read()

    orig_includes = parse_includes(source)
    items = find_functions(source)
    out_dir = os.path.join(ROOT, module)
    os.makedirs(out_dir, exist_ok=True)

    module_header = {
        "ctype": ["#include <ctype.h>"],
        "string": ["#include <string.h>"],
        "math": ["#include <math.h>", '#include "internal/math_common.h"'],
        "errno": ["#include <errno.h>"],
        "libgen": ["#include <libgen.h>"],
        "sys": [],
        "baos": [],
    }.get(module, orig_includes)

    for chunk in items:
        name = func_name(chunk)
        if not name:
            if "static" in chunk and "=" in chunk:
                common = os.path.join(out_dir, f"{module}_data.c")
                with open(common, "a") as f:
                    f.write("\n".join(module_header) + "\n\n" + chunk + "\n")
            continue

        incs = includes_for(module, chunk, module_header if module in ("ctype", "string", "math", "errno", "libgen") else orig_includes)
        out_path = os.path.join(out_dir, f"{name}.c")
        with open(out_path, "w") as f:
            f.write("\n".join(incs) + "\n\n" + chunk.strip() + "\n")

    os.remove(src_path)
    print(f"split {filename} -> {module}/ ({len(items)} items)")


def main():
    for fname, mod in MONOLITHS.items():
        split_file(fname, mod)


if __name__ == "__main__":
    main()
