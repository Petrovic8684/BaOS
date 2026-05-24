#!/usr/bin/env python3
"""Fix runtime module extraction and internal files."""

import os
import re
import subprocess

REPO = os.path.join(os.path.dirname(__file__), "..")
ROOT = os.path.join(REPO, "runtime", "src")


def git_show(path):
    r = subprocess.run(["git", "show", f"HEAD:{path}"], capture_output=True, text=True, cwd=REPO)
    return r.stdout if r.returncode == 0 else ""


def write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)


def extract_fn(src, name):
    pat = rf"(?ms)^(?:static\s+(?:inline\s+)?)?(?:[\w\s\*]+?)\b{re.escape(name)}\s*\((?:[^()]|\([^()]*\))*\)\s*\{{"
    m = re.search(pat, src)
    if not m:
        pat2 = rf"(?ms)^(?:static\s+(?:inline\s+)?)?(?:[\w\s\*]+?)\b{re.escape(name)}\s*\((?:[^()]|\([^()]*\))*\)\s*\{{[^{{}}]*\}}"
        m = re.search(pat2, src)
        return m.group(0) if m else None
    start = m.start()
    i = m.end() - 1
    d = 0
    ins = inc = esc = False
    while i < len(src):
        c = src[i]
        if ins:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                ins = False
        elif inc:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == "'":
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


def auto_includes(body, base):
    inc = list(base)

    def add(x):
        if x not in inc:
            inc.append(x)

    if re.search(r"\b(malloc|free|realloc|calloc)\b", body):
        add("#include <stdlib.h>")
    if re.search(r"\b(strlen|strcpy|memcpy|strcmp|strcat|strdup|strtok|memset|memmove)\b", body):
        add("#include <string.h>")
    if "errno" in body:
        add("#include <errno.h>")
    if re.search(r"\b(isdigit|isspace|tolower|isalpha|isalnum)\b", body):
        add("#include <ctype.h>")
    if re.search(r"\b(printf|fprintf|fputc|fopen|fscanf|fclose|fread|fwrite|FILE|stdout|stderr|stdin|snprintf|sscanf|read_line)\b", body):
        add("#include <stdio.h>")
    if re.search(r"\b(pow|log|sqrt|exp|fabs|floor|ceil|modf|ldexp|frexp|isnan|isinf|isfinite)\s*\(", body):
        add("#include <math.h>")
    if re.search(r"\b(va_list|va_start|va_end|va_arg)\b", body):
        add("#include <stdarg.h>")
    if re.search(r"\bfs_(where|change_dir|make_dir|list_dir|delete_dir|make_file|delete_file|write_file|read_file)\b", body):
        add('#include "internal/fs_helpers.h"')
    if re.search(r"\bsys_(write|read|sleep|get_cursor)\b", body):
        add('#include "internal/syscalls.h"')
    if re.search(r"\b(tty_|send_move_syscall|clamp_cursor|scroll_up|tty_parse_after_write|update_cursor|redraw_buffer|ensure_space)\b", body):
        add('#include "stdio/tty_internal.h"')
    if re.search(r"\b(alloc_file_slot|free_file_slot|file_table|stdin_ungetc)\b", body):
        add('#include "stdio/file_internal.h"')
    if re.search(r"\b(dir_streams|parse_list_into_dir|find_free_stream)\b", body):
        add('#include "dirent/dirent_internal.h"')
    if re.search(r"\b(fill_tm_from_time|time_from_tm|read_timezone_offset|time_gmtime_buf|time_localtime_buf|time_asctime_buf|time_timezone_offset)\b", body):
        add('#include "time/time_internal.h"')
    if re.search(r"\b(heap_expand|free_list_insert_and_coalesce|heap_init_once)\b", body):
        add('#include "stdlib/heap_internal.h"')
    if re.search(r"\bULONG_MAX\b|\bLONG_MAX\b|\bLONG_MIN\b", body):
        add("#include <limits.h>")
    return inc


def emit(out_dir, fn, includes, body):
    if not body:
        print(f"MISSING {out_dir}/{fn}")
        return
    write_file(os.path.join(ROOT, out_dir, f"{fn}.c"), "\n".join(auto_includes(body, includes)) + "\n\n" + body.strip() + "\n")


def split_module(git_path, out_dir, funcs, includes):
    src = git_show(git_path)
    for fn in funcs:
        emit(out_dir, fn, includes, extract_fn(src, fn))


def rebuild_fs_helpers():
    unistd = git_show("runtime/src/unistd.c")
    dirent = git_show("runtime/src/dirent.c")
    stdio = git_show("runtime/src/stdio.c")
    parts = []
    for name in ["fs_where_len", "fs_where", "fs_change_dir", "fs_make_dir", "fs_delete_dir", "fs_list_dir_len", "fs_list_dir"]:
        b = extract_fn(unistd, name) or extract_fn(dirent, name)
        if b:
            parts.append(re.sub(r"^static\s+inline\s+", "", b, flags=re.M).replace("static ", ""))
    for name in ["fs_make_file", "fs_delete_file", "fs_write_file", "fs_read_file", "fs_read_file_size"]:
        b = extract_fn(stdio, name)
        if b:
            parts.append(b.replace("static inline ", ""))
    write_file(
        os.path.join(ROOT, "internal/fs_helpers.c"),
        '#include "internal/fs_helpers.h"\n#include "internal/syscalls.h"\n#include <stdlib.h>\n#include <string.h>\n\n'
        + "\n\n".join(parts)
        + "\n",
    )


def rebuild_tty_internal():
    stdio = git_show("runtime/src/stdio.c")
    header = """#define TTY_ROWS 25
#define TTY_COLS 80
#define TTY_LAST_ROW (TTY_ROWS - 1)
#define TTY_LAST_COL (TTY_COLS - 1)

enum ansi_state
{
    ANSI_NORMAL_ST = 0,
    ANSI_ESC_ST,
    ANSI_CSI_ST
};

static int tty_row = 0;
static int tty_col = 0;
static unsigned char tty_attr = 0x07;
static int ansi_state = ANSI_NORMAL_ST;
static char ansi_params[64];
static int ansi_params_len = 0;
"""
    funcs = []
    for fn in [
        "send_move_syscall", "clamp_cursor", "scroll_up", "tty_wrap_char", "tty_parse_after_write",
        "update_cursor", "redraw_buffer", "ensure_space_and_scroll",
    ]:
        b = extract_fn(stdio, fn)
        if b:
            funcs.append(b.replace("static ", ""))
    write_file(
        os.path.join(ROOT, "stdio/tty_internal.c"),
        '#include "stdio/tty_internal.h"\n#include "internal/syscalls.h"\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n'
        + header
        + "\n\n".join(funcs)
        + "\n",
    )


def rebuild_file_internal():
    stdio = git_show("runtime/src/stdio.c")
    write_file(
        os.path.join(ROOT, "stdio/file_internal.c"),
        '#include "stdio/file_internal.h"\n#include <stdlib.h>\n#include <string.h>\n\n'
        "#define MAX_OPEN_FILES 16\n\n"
        "static FILE file_table[MAX_OPEN_FILES];\n\n"
        "int stdin_ungetc = -1;\n\n"
        + extract_fn(stdio, "alloc_file_slot").replace("static ", "")
        + "\n\n"
        + extract_fn(stdio, "free_file_slot").replace("static ", "")
        + "\n",
    )


def rebuild_heap_internal():
    stdlib = git_show("runtime/src/stdlib.c")
    types = re.search(r"typedef struct free_hdr[\s\S]*?} alloc_hdr_t;", stdlib).group(0)
    vars_ = []
    for v in ["free_list", "heap_start", "heap_end", "heap_max"]:
        m = re.search(rf"static [\s\S]*?\b{v}\b[\s\S]*?;", stdlib)
        if m:
            vars_.append(m.group(0))
    body = types + "\n\n"
    body += "#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))\n"
    body += "#define ALLOC_ALIGN 8\n#define PAGE_SIZE_LOCAL 4096\n"
    body += "#define USER_STACK_TOP 0x02100000U\n#define USER_STACK_PAGES 4U\n\n"
    body += "\n".join(vars_) + "\n\n"
    for fn in ["sys_set_user_pages", "heap_expand", "free_list_insert_and_coalesce", "heap_init_once"]:
        b = extract_fn(stdlib, fn)
        if b:
            body += b.replace("static ", "") + "\n\n"
    write_file(
        os.path.join(ROOT, "stdlib/heap_internal.c"),
        '#include "stdlib/heap_internal.h"\n#include "internal/syscalls.h"\n#include <errno.h>\n#include <string.h>\n\n'
        + body,
    )


def main():
    rebuild_fs_helpers()
    rebuild_tty_internal()
    rebuild_file_internal()
    rebuild_heap_internal()

    split_module("runtime/src/string.c", "string",
        ["strlen", "strcmp", "strncmp", "strcoll", "strxfrm", "strcpy", "strncpy", "strcat", "strncat",
         "strchr", "strrchr", "strstr", "strspn", "strcspn", "strpbrk", "memset", "memcpy", "memmove",
         "memchr", "memcmp", "strtok_r", "strtok", "strdup", "stpcpy", "stpncpy"],
        ["#include <string.h>"])

    split_module("runtime/src/dirent.c", "dirent",
        ["opendir", "readdir", "closedir", "rewinddir", "telldir", "seekdir"],
        ["#include <dirent.h>", '#include "internal/fs_helpers.h"', '#include "dirent/dirent_internal.h"'])

    split_module("runtime/src/unistd.c", "unistd",
        ["chdir", "getcwd", "rmdir", "sleep", "usleep"],
        ["#include <unistd.h>", '#include "internal/syscalls.h"', '#include "internal/fs_helpers.h"'])

    split_module("runtime/src/time.c", "time",
        ["time", "mktime", "difftime"],
        ["#include <time.h>", '#include "internal/syscalls.h"'])

    split_module("runtime/src/libgen.c", "libgen", ["basename", "dirname"],
        ["#include <libgen.h>", "#include <string.h>"])

    split_module("runtime/src/stdlib.c", "stdlib",
        ["exit", "abort", "strtod", "strtoul", "strtol", "malloc", "free", "realloc", "calloc",
         "srand", "rand", "atof", "atoi", "atol", "abs", "labs", "llabs", "div", "ldiv", "lldiv",
         "bsearch", "qsort", "realpath"],
        ["#include <stdlib.h>"])

    split_module("runtime/src/stdio.c", "stdio",
        ["write", "fputc", "putchar", "fputs", "puts", "vfprintf", "fprintf", "printf",
         "getchar", "ungetc", "fgetc", "read_line", "scanf", "fscanf", "sscanf", "fflush",
         "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "rewind", "remove", "rename",
         "feof", "ferror", "clearerr", "setvbuf", "setbuf",
         "vsnprintf", "vsprintf", "snprintf", "sprintf"],
        ["#include <stdio.h>", '#include "internal/syscalls.h"'])

    split_module("runtime/src/math.c", "math",
        ["isnan", "isinf", "isfinite", "fabs", "frexp", "ldexp", "modf", "floor", "ceil", "sqrt", "fmod",
         "pow", "exp", "log", "log10", "sin", "cos", "tan", "atan", "atan2", "asin", "acos", "sinh", "cosh",
         "tanh", "hypot", "acosh", "asinh", "atanh", "log1p", "expm1", "cbrt", "round", "trunc", "fma", "nan", "inf", "fmax", "fmin"],
        ['#include <math.h>', '#include "internal/math_common.h"'])

    print("runtime modules fixed")


if __name__ == "__main__":
    main()
