#!/usr/bin/env python3
"""Full BaOS runtime modularization."""

import os
import re
import shutil

ROOT = os.path.join(os.path.dirname(__file__), "..", "runtime", "src")


def read(p):
    with open(p) as f:
        return f.read()


def write(p, s):
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w") as f:
        f.write(s)


def extract_fn(src, name):
    for prefix in [
        rf"static\s+inline\s+",
        rf"static\s+",
        rf"",
    ]:
        pat = (
            rf"({prefix}(?:void|int|char|unsigned|double|long|long long|size_t|time_t|"
            rf"FILE|DIR|div_t|ldiv_t|lldiv_t|struct\s+\w+\s*\*?)\s+\*?\s*{re.escape(name)}\s*"
            rf"\([^{{]*\)\s*\{{)"
        )
        m = re.search(pat, src)
        if m:
            start = m.start()
            i = m.end() - 1
            d = 0
            ins = inc = esc = False
            while i < len(src):
                c = src[i]
                if ins:
                    esc = c == "\\" and not esc if not esc else False
                    if c == '"' and not esc:
                        ins = False
                elif inc:
                    esc = c == "\\" and not esc if not esc else False
                    if c == "'" and not esc:
                        inc = False
                elif c == '"':
                    ins = True
                elif c == "'":
                    inc = True
                elif c == "{":
                    d += 1
                elif c == "}":
                    d -= 1
                    if d == 0:
                        return src[start : i + 1]
                i += 1
    return None


def split_funcs(monolith, out_dir, funcs, includes, header=""):
    src = read(os.path.join(ROOT, monolith))
    os.makedirs(os.path.join(ROOT, out_dir), exist_ok=True)
    if header:
        write(os.path.join(ROOT, out_dir, "_common.c"), header + "\n")
    for fn in funcs:
        body = extract_fn(src, fn)
        if not body:
            print(f"WARN: {fn} not found in {monolith}")
            continue
        inc = list(includes)
        if any(x in body for x in ("malloc", "free", "realloc", "calloc")):
            if "#include <stdlib.h>" not in inc:
                inc.append("#include <stdlib.h>")
        if any(x in body for x in ("strlen", "strcpy", "memcpy", "strcmp", "strcat", "strdup", "strtok")):
            if "#include <string.h>" not in inc:
                inc.append("#include <string.h>")
        if "errno" in body and "#include <errno.h>" not in inc:
            inc.append("#include <errno.h>")
        if any(x in body for x in ("isdigit", "isspace", "tolower", "isalpha")):
            if "#include <ctype.h>" not in inc:
                inc.append("#include <ctype.h>")
        if any(x in body for x in ("printf", "fprintf", "fputc", "fopen", "fscanf", "FILE")):
            if "#include <stdio.h>" not in inc:
                inc.append("#include <stdio.h>")
        if any(x in body for x in ("pow(", "log(", "sqrt(", "exp(", "fabs(", "floor(", "ceil(", "modf(", "ldexp(", "frexp(", "isnan(", "isinf(", "isfinite(")):
            if "#include <math.h>" not in inc:
                inc.append("#include <math.h>")
        if "map_fs_error" in body:
            inc.append("#include <errno.h>")
        if "fs_where" in body or "fs_change_dir" in body or "fs_list_dir" in body:
            inc.append('#include "internal/fs_helpers.h"')
        if "va_list" in body or "va_start" in body:
            inc.append("#include <stdarg.h>")
        write(os.path.join(ROOT, out_dir, f"{fn}.c"), "\n".join(inc) + "\n\n" + body + "\n")
    os.remove(os.path.join(ROOT, monolith))


def create_internals():
    write(
        os.path.join(ROOT, "internal/syscalls.h"),
        read(os.path.join(os.path.dirname(__file__), "runtime_internal_syscalls.h")),
    )


def main():
    # ctype - one fn per file, fix EOF
    ctype_src = read(os.path.join(ROOT, "ctype.c")).replace("#include <stdio.h>", "#ifndef EOF\n#define EOF (-1)\n#endif")
    write(os.path.join(ROOT, "ctype.c"), ctype_src)
    split_funcs(
        "ctype.c",
        "ctype",
        [
            "isalnum", "isalpha", "iscntrl", "isdigit", "isgraph", "islower", "isupper",
            "ispunct", "isspace", "isprint", "isxdigit", "tolower", "toupper", "isblank",
            "isascii", "toascii",
        ],
        ["#include <ctype.h>", "#ifndef EOF", "#define EOF (-1)", "#endif"],
    )

    # string
    split_funcs(
        "string.c",
        "string",
        [
            "strlen", "strcmp", "strncmp", "strcoll", "strxfrm", "strcpy", "strncpy",
            "strcat", "strncat", "strchr", "strrchr", "strstr", "strspn", "strcspn",
            "strpbrk", "memset", "memcpy", "memmove", "memchr", "memcmp",
            "strtok_r", "strtok", "strdup", "stpcpy", "stpncpy",
        ],
        ["#include <string.h>"],
    )

    # math - common header first
    math_src = read(os.path.join(ROOT, "math.c"))
    common_end = math_src.find("int isnan")
    math_common = math_src[:common_end].strip()
    write(
        os.path.join(ROOT, "internal/math_common.h"),
        math_common.replace("static inline", "static inline") + "\n\n"
        "static inline int is_integer_double(double x, int *ival);\n",
    )
    # add is_integer_double to common.c
    iid = extract_fn(math_src, "is_integer_double")
    write(
        os.path.join(ROOT, "internal/math_common.c"),
        '#include "internal/math_common.h"\n\n' + (iid or "") + "\n",
    )
    split_funcs(
        "math.c",
        "math",
        [
            "isnan", "isinf", "isfinite", "fabs", "frexp", "ldexp", "modf", "floor", "ceil",
            "sqrt", "fmod", "pow", "exp", "log", "log10", "sin", "cos", "tan", "atan", "atan2",
            "asin", "acos", "sinh", "cosh", "tanh", "hypot", "acosh", "asinh", "atanh",
            "log1p", "expm1", "cbrt", "round", "trunc", "fma", "nan", "inf", "fmax", "fmin",
        ],
        ['#include <math.h>', '#include "internal/math_common.h"'],
    )

    # reduce_periodic stays in sin.c - extract manually
    rp = extract_fn(math_src, "reduce_periodic")
    if rp:
        append = read(os.path.join(ROOT, "math/sin.c"))
        write(os.path.join(ROOT, "math/_trig_common.c"), '#include <math.h>\n#include "internal/math_common.h"\n\n' + rp + "\n")

    print("done batch 1")


if __name__ == "__main__":
    main()
