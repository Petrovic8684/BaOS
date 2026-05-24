#!/usr/bin/env python3
"""Modularize BaOS runtime into fine-grained .c files."""

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
    pat = rf"((?:static\s+(?:inline\s+)?)?(?:const\s+)?(?:unsigned\s+|signed\s+)?(?:long\s+long\s+|long\s+|short\s+)?(?:struct\s+\w+\s*\*?\s+|void\s*\*?\s+|char\s*\*?\s+|int\s+\*?\s*|size_t\s+|double\s+|float\s+|FILE\s*\*?\s+|DIR\s*\*?\s+|div_t\s+|ldiv_t\s+|lldiv_t\s+|time_t\s+|struct\s+tm\s*\*?\s+|struct\s+dirent\s*\*?\s+)?{re.escape(name)}\s*\([^{{]*\)\s*\{{)"
    m = re.search(pat, src)
    if not m:
        return None
    start = m.start()
    i = m.end() - 1
    d = 0
    ins = inc = False
    esc = False
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


def extract_static_var_block(src, varname):
    m = re.search(rf"(static\s+[\s\S]*?\b{re.escape(varname)}\b[\s\S]*?;)", src)
    return m.group(1) if m else ""


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
    if re.search(r"\b(printf|fprintf|fputc|fopen|fscanf|fclose|fread|fwrite|FILE|stdout|stderr|stdin)\b", body):
        add("#include <stdio.h>")
    if re.search(r"\b(pow|log|sqrt|exp|fabs|floor|ceil|modf|ldexp|frexp|isnan|isinf|isfinite)\s*\(", body):
        add("#include <math.h>")
    if re.search(r"\b(va_list|va_start|va_end|va_arg)\b", body):
        add("#include <stdarg.h>")
    if "map_fs_error" in body:
        add("#include <errno.h>")
    if re.search(r"\bfs_(where|change_dir|list_dir|delete_dir|make_file|delete_file|write_file|read_file)\b", body):
        add('#include "internal/fs_helpers.h"')
    if re.search(r"\b(tty_|send_move_syscall|clamp_cursor|scroll_up|tty_parse_after_write|update_cursor|redraw_buffer)\b", body):
        add('#include "stdio/tty_internal.h"')
    if re.search(r"\b(file_table|alloc_file_slot|free_file_slot|stdin_ungetc)\b", body):
        add('#include "stdio/file_internal.h"')
    if re.search(r"\b(dir_streams|parse_list_into_dir|find_free_stream)\b", body):
        add('#include "dirent/dirent_internal.h"')
    if re.search(r"\b(fill_tm_from_time|time_from_tm|civil_from_days|is_leap|gmtime_buf|localtime_buf|asctime_buf|timezone_offset)\b", body):
        add('#include "time/time_internal.h"')
    if re.search(r"\b(heap_|free_list|malloc|_end|sys_set_user_pages)\b", body) and "stdlib/heap_internal.h" not in body:
        if any(x in body for x in ("heap_expand", "free_list_insert", "heap_init_once", "free_list", "heap_start")):
            add('#include "stdlib/heap_internal.h"')
    if re.search(r"\b(fmt_emit_|vfprintf_handle)\b", body):
        add('#include "stdio/fmt_internal.h"')
    return inc


def emit(out_dir, fn, includes, body):
    if not body:
        print(f"  MISSING {fn}")
        return
    inc = auto_includes(body, includes)
    write(os.path.join(ROOT, out_dir, f"{fn}.c"), "\n".join(inc) + "\n\n" + body.strip() + "\n")


def split_list(monolith, out_dir, funcs, includes):
    src = read(os.path.join(ROOT, monolith))
    for fn in funcs:
        emit(out_dir, fn, includes, extract_fn(src, fn))
    os.remove(os.path.join(ROOT, monolith))


def move_baos_module(monolith, dest):
    src_path = os.path.join(ROOT, monolith)
    if os.path.exists(src_path):
        write(os.path.join(ROOT, dest), read(src_path))
        os.remove(src_path)


def setup_internal():
    write(
        os.path.join(ROOT, "internal/syscalls.h"),
        """#ifndef BAOS_INTERNAL_SYSCALLS_H
#define BAOS_INTERNAL_SYSCALLS_H

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_RTC_NOW 5
#define SYS_FS_WHERE 8
#define SYS_FS_LIST_DIR 9
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_DELETE_DIR 12
#define SYS_FS_MAKE_FILE 13
#define SYS_FS_DELETE_FILE 14
#define SYS_FS_WRITE_FILE 15
#define SYS_FS_READ_FILE 16
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19
#define SYS_SET_USER_PAGES 21
#define SYS_SLEEP 23

static inline void sys_write(const char *str)
{
    __asm__ volatile(
        "movl %[num], %%eax\\n\\t"
        "movl %[s], %%ebx\\n\\t"
        "int $0x80\\n\\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline unsigned char sys_read(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\\n\\t"
        "int $0x80\\n\\t"
        "movl %%ebx, %[res]\\n\\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_READ)
        : "eax", "ebx", "memory");
    return (unsigned char)(ret & 0xFF);
}

static inline int sys_get_cursor_row(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\\n\\t"
        "int $0x80\\n\\t"
        "movl %%ebx, %[res]\\n\\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_ROW)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_get_cursor_col(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\\n\\t"
        "int $0x80\\n\\t"
        "movl %%ebx, %[res]\\n\\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_COL)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline void sys_sleep_ms(unsigned int ms)
{
    __asm__ volatile(
        "movl %[num], %%eax\\n\\t"
        "movl %[arg], %%ebx\\n\\t"
        "int $0x80\\n\\t"
        :
        : [num] "i"(SYS_SLEEP), [arg] "r"(ms)
        : "eax", "ebx", "memory");
}

#endif
""",
    )

    unistd = read(os.path.join(ROOT, "unistd.c"))
    dirent = read(os.path.join(ROOT, "dirent.c"))
    stdio = read(os.path.join(ROOT, "stdio.c"))

    fs_parts = []
    for name in [
        "fs_where_len", "fs_where", "fs_change_dir", "fs_delete_dir",
        "fs_list_dir_len", "fs_list_dir",
    ]:
        b = extract_fn(unistd, name) or extract_fn(dirent, name)
        if b:
            fs_parts.append(b)

    for name in ["fs_make_file", "fs_delete_file", "fs_write_file", "fs_read_file", "fs_read_file_size"]:
        b = extract_fn(stdio, name)
        if b:
            fs_parts.append(b.replace("static inline ", ""))

    write(
        os.path.join(ROOT, "internal/fs_helpers.h"),
        """#ifndef BAOS_INTERNAL_FS_HELPERS_H
#define BAOS_INTERNAL_FS_HELPERS_H

#include <errno.h>

int map_fs_error(int fs_error);

unsigned int fs_where_len(void);
char *fs_where(void);
int fs_change_dir(const char *name);
int fs_delete_dir(const char *name);
unsigned int fs_list_dir_len(void);
char *fs_list_dir(void);
int fs_make_file(const char *name);
int fs_delete_file(const char *name);
int fs_write_file(const char *name, const unsigned char *data, unsigned int size);
int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size);
int fs_read_file_size(const char *name);

#endif
""",
    )

    write(
        os.path.join(ROOT, "internal/fs_helpers.c"),
        '#include "internal/fs_helpers.h"\n#include "internal/syscalls.h"\n'
        "#include <stdlib.h>\n#include <string.h>\n\n"
        + "\n\n".join(fs_parts)
        + "\n",
    )


def setup_stdio_internals():
    stdio = read(os.path.join(ROOT, "stdio.c"))
    tty_fns = [
        "send_move_syscall", "clamp_cursor", "scroll_up", "tty_wrap_char",
        "tty_parse_after_write", "update_cursor", "redraw_buffer",
        "ensure_space_and_scroll",
    ]
    tty_vars = re.findall(r"^static .+?;", stdio, re.M)
    tty_enum = extract_static_var_block(stdio, "ansi_state")
    # grab enum separately
    enum_m = re.search(r"enum ansi_state[\s\S]*?};", stdio)
    enum_block = enum_m.group(0) if enum_m else ""

    static_vars = []
    for v in ["tty_row", "tty_col", "tty_attr", "ansi_state", "ansi_params", "ansi_params_len"]:
        m = re.search(rf"static [\s\S]*?\b{v}\b[\s\S]*?;", stdio)
        if m:
            static_vars.append(m.group(0))

    write(
        os.path.join(ROOT, "stdio/tty_internal.h"),
        """#ifndef BAOS_STDIO_TTY_INTERNAL_H
#define BAOS_STDIO_TTY_INTERNAL_H

#include <stdio.h>

void send_move_syscall(int r, int c);
void clamp_cursor(int *row, int *col);
void scroll_up(int n);
void tty_wrap_char(unsigned char ch);
void tty_parse_after_write(const char *s);
void update_cursor(int new_row, int new_col);
void redraw_buffer(const char *buf, int len, int start_row, int start_col);
void ensure_space_and_scroll(int *start_row, int start_col, int cur_len, int old_len);

#endif
""",
    )

    tty_body = enum_block + "\n\n" + "\n".join(static_vars) + "\n\n"
    for fn in tty_fns:
        b = extract_fn(stdio, fn)
        if b:
            tty_body += b + "\n\n"

    write(
        os.path.join(ROOT, "stdio/tty_internal.c"),
        '#include "stdio/tty_internal.h"\n#include "internal/syscalls.h"\n'
        "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n"
        + tty_body,
    )

    file_vars = []
    m = re.search(r"static FILE file_table\[MAX_OPEN_FILES\];", stdio)
    if m:
        file_vars.append("#define MAX_OPEN_FILES 16\n" + m.group(0))
    m = re.search(r"static int stdin_ungetc = -1;", stdio)
    if m:
        file_vars.append(m.group(0))

    write(
        os.path.join(ROOT, "stdio/file_internal.h"),
        """#ifndef BAOS_STDIO_FILE_INTERNAL_H
#define BAOS_STDIO_FILE_INTERNAL_H

#include <stdio.h>

#define MAX_OPEN_FILES 16

FILE *alloc_file_slot(void);
void free_file_slot(FILE *f);
extern int stdin_ungetc;

#endif
""",
    )

    file_body = "\n".join(file_vars) + "\n\n"
    for fn in ["alloc_file_slot", "free_file_slot"]:
        b = extract_fn(stdio, fn)
        if b:
            file_body += b + "\n\n"

    write(
        os.path.join(ROOT, "stdio/file_internal.c"),
        '#include "stdio/file_internal.h"\n#include <stdlib.h>\n#include <string.h>\n\n'
        + file_body,
    )


def setup_heap_internal():
    stdlib = read(os.path.join(ROOT, "stdlib.c"))
    types = re.search(
        r"typedef struct free_hdr[\s\S]*?} alloc_hdr_t;", stdlib
    )
    vars_block = ""
    for v in ["free_list", "heap_start", "heap_end", "heap_max"]:
        m = re.search(rf"static [\s\S]*?\b{v}\b[\s\S]*?;", stdlib)
        if m:
            vars_block += m.group(0) + "\n"

    write(
        os.path.join(ROOT, "stdlib/heap_internal.h"),
        """#ifndef BAOS_STDLIB_HEAP_INTERNAL_H
#define BAOS_STDLIB_HEAP_INTERNAL_H

#include <stdlib.h>

extern unsigned int _end;

void *heap_expand(unsigned int bytes);
void free_list_insert_and_coalesce(void *blk);
void heap_init_once(void);

#endif
""",
    )

    heap_body = ""
    if types:
        heap_body += types.group(0) + "\n\n"
    heap_body += "#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))\n"
    heap_body += "#define ALLOC_ALIGN 8\n#define PAGE_SIZE_LOCAL 4096\n"
    heap_body += "#define USER_STACK_TOP 0x02100000U\n#define USER_STACK_PAGES 4U\n\n"
    heap_body += vars_block + "\n"
    for fn in ["sys_set_user_pages", "heap_expand", "free_list_insert_and_coalesce", "heap_init_once"]:
        b = extract_fn(stdlib, fn)
        if b:
            heap_body += b + "\n\n"

    write(
        os.path.join(ROOT, "stdlib/heap_internal.c"),
        '#include "stdlib/heap_internal.h"\n#include "internal/syscalls.h"\n'
        "#include <errno.h>\n#include <string.h>\n\n" + heap_body,
    )


def setup_dirent_internal():
    dirent = read(os.path.join(ROOT, "dirent.c"))
    struct_m = re.search(r"struct DIR[\s\S]*?};", dirent)
    streams_m = re.search(r"static struct DIR dir_streams\[DIRENT_MAX_STREAMS\];", dirent)

    write(
        os.path.join(ROOT, "dirent/dirent_internal.h"),
        """#ifndef BAOS_DIRENT_INTERNAL_H
#define BAOS_DIRENT_INTERNAL_H

#include <dirent.h>

int find_free_stream(void);
unsigned int parse_list_into_dir(DIR *d, const char *list);

#endif
""",
    )

    body = ""
    if struct_m:
        body += struct_m.group(0) + "\n\n"
    if streams_m:
        body += streams_m.group(0) + "\n\n"
    for fn in ["find_free_stream", "parse_list_into_dir"]:
        b = extract_fn(dirent, fn)
        if b:
            body += b + "\n\n"

    write(
        os.path.join(ROOT, "dirent/dirent_internal.c"),
        '#include "dirent/dirent_internal.h"\n#include "internal/fs_helpers.h"\n'
        "#include <stdlib.h>\n#include <string.h>\n\n" + body,
    )


def setup_time_internal():
    time_src = read(os.path.join(ROOT, "time.c"))
    write(
        os.path.join(ROOT, "time/time_internal.h"),
        """#ifndef BAOS_TIME_INTERNAL_H
#define BAOS_TIME_INTERNAL_H

#include <time.h>

void fill_tm_from_time(time_t t, struct tm *out);
time_t time_from_tm(const struct tm *tm);
int is_leap(int y);
void civil_from_days(int z, int *y, int *m, int *d);
short get_timezone_offset(void);

#endif
""",
    )

    static_vars = []
    for v in ["timezone_offset", "gmtime_buf", "localtime_buf", "asctime_buf"]:
        m = re.search(rf"static [\s\S]*?\b{v}\b[\s\S]*?;", time_src)
        if m:
            static_vars.append(m.group(0))

    body = "\n".join(static_vars) + "\n\n"
    for fn in ["is_leap", "civil_from_days", "fill_tm_from_time", "time_from_tm", "read_timezone_offset"]:
        b = extract_fn(time_src, fn)
        if b:
            body += b + "\n\n"

    write(
        os.path.join(ROOT, "time/time_internal.c"),
        '#include "time/time_internal.h"\n#include <stdio.h>\n#include <string.h>\n\n' + body,
    )


def setup_errno():
    errno_src = read(os.path.join(ROOT, "errno.c"))
    errlist_m = re.search(r"static const char \*const __sys_errlist\[\] = \{[\s\S]*?\};", errno_src)
    nerr_m = re.search(r"static const int __sys_nerr = .*?;", errno_src)
    loc = extract_fn(errno_src, "__errno_location")
    common = ""
    if loc:
        common += "static int _errno_var = 0;\n\n" + loc + "\n\n"
    if errlist_m:
        common += errlist_m.group(0) + "\n\n"
    if nerr_m:
        common += nerr_m.group(0) + "\n"
    write(
        os.path.join(ROOT, "errno/_common.c"),
        "#include <errno.h>\n\n" + common,
    )
    perror_b = extract_fn(errno_src, "perror")
    emit("stdio", "perror", ["#include <stdio.h>", "#include <errno.h>"], perror_b)
    split_list("errno.c", "errno", ["map_fs_error", "strerror"], ["#include <errno.h>"])


def setup_math_common():
    math_src = read(os.path.join(ROOT, "math.c"))
    header_end = math_src.find("int isnan")
    common = math_src[:header_end].strip()
    write(os.path.join(ROOT, "internal/math_common.h"), common + "\n")
    iid = extract_fn(math_src, "is_integer_double")
    rp = extract_fn(math_src, "reduce_periodic")
    write(
        os.path.join(ROOT, "internal/math_common.c"),
        '#include "internal/math_common.h"\n\n' + (iid or "") + "\n\n" + (rp or "") + "\n",
    )


def main():
    setup_internal()
    setup_math_common()
    setup_stdio_internals()
    setup_heap_internal()
    setup_dirent_internal()
    setup_time_internal()

    # ctype
    ctype = read(os.path.join(ROOT, "ctype.c")).replace("#include <stdio.h>", "")
    write(os.path.join(ROOT, "ctype.c"), ctype)
    split_list(
        "ctype.c", "ctype",
        ["isalnum", "isalpha", "iscntrl", "isdigit", "isgraph", "islower", "isupper",
         "ispunct", "isspace", "isprint", "isxdigit", "tolower", "toupper", "isblank", "isascii", "toascii"],
        ["#include <ctype.h>", "#ifndef EOF", "#define EOF (-1)", "#endif"],
    )

    split_list(
        "string.c", "string",
        ["strlen", "strcmp", "strncmp", "strcoll", "strxfrm", "strcpy", "strncpy", "strcat", "strncat",
         "strchr", "strrchr", "strstr", "strspn", "strcspn", "strpbrk", "memset", "memcpy", "memmove",
         "memchr", "memcmp", "strtok_r", "strtok", "strdup", "stpcpy", "stpncpy"],
        ["#include <string.h>"],
    )

    split_list(
        "math.c", "math",
        ["isnan", "isinf", "isfinite", "fabs", "frexp", "ldexp", "modf", "floor", "ceil", "sqrt", "fmod",
         "pow", "exp", "log", "log10", "sin", "cos", "tan", "atan", "atan2", "asin", "acos", "sinh", "cosh",
         "tanh", "hypot", "acosh", "asinh", "atanh", "log1p", "expm1", "cbrt", "round", "trunc", "fma", "nan", "inf", "fmax", "fmin"],
        ['#include <math.h>', '#include "internal/math_common.h"'],
    )

    setup_errno()

    split_list(
        "libgen.c", "libgen",
        ["basename", "dirname"],
        ["#include <libgen.h>", "#include <string.h>"],
    )

    split_list("sys_info.c", "sys", ["sysinfo"], ["#include <sys/sysinfo.h>"])
    split_list("sys_stat.c", "sys", ["mkdir", "stat"], ["#include <sys/stat.h>", "#include <dirent.h>", "#include <string.h>", "#include <stdio.h>", "#include <errno.h>"])
    split_list("sys_utsname.c", "sys", ["uname"], ["#include <sys/utsname.h>"])

    move_baos_module("baos_mouse.c", "baos/mouse.c")
    move_baos_module("baos_vga.c", "baos/vga.c")
    move_baos_module("baos_sound.c", "baos/sound.c")

    # stdlib public funcs
    split_list(
        "stdlib.c", "stdlib",
        ["exit", "abort", "strtod", "strtoul", "strtol", "malloc", "free", "realloc", "calloc",
         "srand", "rand", "atof", "atoi", "atol", "abs", "labs", "llabs", "div", "ldiv", "lldiv",
         "bsearch", "qsort", "realpath"],
        ["#include <stdlib.h>"],
    )

    # unistd - only public, fs helpers moved
    split_list(
        "unistd.c", "unistd",
        ["chdir", "getcwd", "rmdir", "sleep", "usleep"],
        ["#include <unistd.h>", '#include "internal/fs_helpers.h"'],
    )

    # dirent public
    split_list(
        "dirent.c", "dirent",
        ["opendir", "readdir", "closedir", "rewinddir", "telldir", "seekdir"],
        ["#include <dirent.h>", '#include "internal/fs_helpers.h"', '#include "dirent/dirent_internal.h"'],
    )

    # time public
    split_list(
        "time.c", "time",
        ["time", "gmtime", "localtime", "mktime", "asctime", "ctime", "difftime"],
        ["#include <time.h>", '#include "time/time_internal.h"'],
    )

    # stdio public - split vfprintf fmt handlers
    stdio = read(os.path.join(ROOT, "stdio.c"))
    split_list(
        "stdio.c", "stdio",
        [
            "write", "fputc", "putchar", "fputs", "puts", "vfprintf", "fprintf", "printf",
            "getchar", "ungetc", "fgetc", "read_line", "scanf", "fscanf", "sscanf", "fflush",
            "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "rewind", "remove", "rename",
            "feof", "ferror", "clearerr", "setvbuf", "setbuf",
            "vsnprintf", "vsprintf", "snprintf", "sprintf",
        ],
        ["#include <stdio.h>", '#include "internal/syscalls.h"'],
    )

    print("Runtime modularization complete.")


if __name__ == "__main__":
    main()
