#!/usr/bin/env python3
"""Rebuild corrupted runtime internal modules and fix includes."""

import os
import re
import subprocess

REPO = os.path.join(os.path.dirname(__file__), "..")
ROOT = os.path.join(REPO, "runtime", "src")


def git_show(path):
    r = subprocess.run(
        ["git", "show", f"HEAD:{path}"],
        capture_output=True,
        text=True,
        cwd=REPO,
    )
    return r.stdout if r.returncode == 0 else ""


def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)


def extract_fn(src, name):
    pat = rf"((?:static\s+(?:inline\s+)?)?(?:[\w\s\*]+?)\b{re.escape(name)}\s*\([^{{]*\)\s*\{{)"
    for m in re.finditer(pat, src):
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
    pat2 = rf"((?:static\s+(?:inline\s+)?)?(?:[\w\s\*]+?)\b{re.escape(name)}\s*\([^{{]*\)\s*\{{[^{{}}]*\}})"
    m = re.search(pat2, src)
    return m.group(1) if m else None


def extract_block(src, start_pat):
    m = re.search(start_pat, src, re.M)
    if not m:
        return ""
    start = m.start()
    if "{" not in m.group(0):
        end = src.find(";", m.end())
        return src[start : end + 1] if end >= 0 else ""
    i = src.find("{", m.start())
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
    return ""


def rebuild_fs_errors():
    write(
        os.path.join(ROOT, "internal/fs_errors.h"),
        """#ifndef BAOS_INTERNAL_FS_ERRORS_H
#define BAOS_INTERNAL_FS_ERRORS_H

#define FS_OK 0
#define FS_ERR_NO_DRV -1
#define FS_ERR_IO -2
#define FS_ERR_EXISTS -3
#define FS_ERR_NO_SPACE -4
#define FS_ERR_NAME_LONG -5
#define FS_ERR_NOT_INIT -6
#define FS_ERR_NO_NAME -7
#define FS_ERR_NO_TEXT -8
#define FS_ERR_NOT_EXISTS -9

#endif
""",
    )


def rebuild_fs_helpers():
    unistd = git_show("runtime/src/unistd.c")
    dirent = git_show("runtime/src/dirent.c")
    stdio = git_show("runtime/src/stdio.c")
    parts = []
    for name in ["fs_where_len", "fs_where", "fs_change_dir", "fs_delete_dir", "fs_list_dir_len", "fs_list_dir"]:
        b = extract_fn(unistd, name) or extract_fn(dirent, name)
        if b:
            parts.append(b.replace("static ", "").replace("static inline ", ""))
    for name in ["fs_make_file", "fs_delete_file", "fs_write_file", "fs_read_file", "fs_read_file_size"]:
        b = extract_fn(stdio, name)
        if b:
            parts.append(b.replace("static inline ", ""))
    write(
        os.path.join(ROOT, "internal/fs_helpers.c"),
        '#include "internal/fs_helpers.h"\n#include "internal/syscalls.h"\n'
        "#include <stdlib.h>\n#include <string.h>\n\n" + "\n\n".join(parts) + "\n",
    )


def rebuild_file_internal():
    stdio = git_show("runtime/src/stdio.c")
    table = extract_block(stdio, r"static FILE file_table\[MAX_OPEN_FILES\]")
    unget = extract_block(stdio, r"static int stdin_ungetc")
    alloc = extract_fn(stdio, "alloc_file_slot")
    free = extract_fn(stdio, "free_file_slot")
    if alloc:
        alloc = alloc.replace("static ", "")
    if free:
        free = free.replace("static ", "")
    write(
        os.path.join(ROOT, "stdio/file_internal.c"),
        '#include "stdio/file_internal.h"\n#include <stdlib.h>\n#include <string.h>\n\n'
        "#define MAX_OPEN_FILES 16\n\n"
        + (table + "\n\n" if table else "")
        + (unget + "\n\n" if unget else "int stdin_ungetc = -1;\n\n")
        + (alloc + "\n\n" if alloc else "")
        + (free + "\n\n" if free else ""),
    )


def rebuild_tty_internal():
    stdio = git_show("runtime/src/stdio.c")
    chunks = []
    for pat in [
        r"#define TTY_ROWS",
        r"enum ansi_state",
        r"static int tty_row",
        r"static int ansi_state",
        r"static char ansi_params",
    ]:
        b = extract_block(stdio, pat)
        if b and b not in chunks:
            chunks.append(b)
    for fn in [
        "send_move_syscall", "clamp_cursor", "scroll_up", "tty_wrap_char",
        "tty_parse_after_write", "update_cursor", "redraw_buffer", "ensure_space_and_scroll",
    ]:
        b = extract_fn(stdio, fn)
        if b:
            b = b.replace("static ", "")
            chunks.append(b)
    write(
        os.path.join(ROOT, "stdio/tty_internal.c"),
        '#include "stdio/tty_internal.h"\n#include "internal/syscalls.h"\n'
        "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdarg.h>\n\n"
        + "\n\n".join(chunks)
        + "\n",
    )


def rebuild_time_internal():
    time_src = git_show("runtime/src/time.c")
    write(
        os.path.join(ROOT, "time/time_internal.h"),
        """#ifndef BAOS_TIME_INTERNAL_H
#define BAOS_TIME_INTERNAL_H

#include <time.h>

#define TIMEZONE_FILE_PATH "/config/timezone"

void fill_tm_from_time(time_t t, struct tm *out);
time_t time_from_tm(const struct tm *tm);
void read_timezone_offset(void);
struct tm *time_gmtime_buf(void);
struct tm *time_localtime_buf(void);
char *time_asctime_buf(void);
short *time_timezone_offset(void);

#endif
""",
    )
    statics = []
    for pat in [r"static short timezone_offset", r"static struct tm gmtime_buf", r"static struct tm localtime_buf", r"static char asctime_buf"]:
        b = extract_block(time_src, pat)
        if b:
            statics.append(b)
    funcs = []
    for fn in ["is_leap", "civil_from_days", "fill_tm_from_time", "time_from_tm", "read_timezone_offset"]:
        b = extract_fn(time_src, fn)
        if b:
            funcs.append(b.replace("static ", ""))
    accessors = """
struct tm *time_gmtime_buf(void) { return &gmtime_buf; }
struct tm *time_localtime_buf(void) { return &localtime_buf; }
char *time_asctime_buf(void) { return asctime_buf; }
short *time_timezone_offset(void) { return &timezone_offset; }
"""
    write(
        os.path.join(ROOT, "time/time_internal.c"),
        '#include "time/time_internal.h"\n#include <stdio.h>\n#include <string.h>\n\n'
        + "\n\n".join(statics)
        + "\n\n"
        + "\n\n".join(funcs)
        + "\n"
        + accessors,
    )


def rebuild_errno():
    errno_src = git_show("runtime/src/errno.c")
    write(
        os.path.join(ROOT, "errno/errno_internal.h"),
        """#ifndef BAOS_ERRNO_INTERNAL_H
#define BAOS_ERRNO_INTERNAL_H

extern const char *const __sys_errlist[];
extern const int __sys_nerr;

#endif
""",
    )
    errlist = re.search(r"static const char \*const __sys_errlist\[\] = \{[\s\S]*?\};", errno_src)
    nerr = re.search(r"static const int __sys_nerr = .*?;", errno_src)
    loc = extract_fn(errno_src, "__errno_location")
    common = "#include <errno.h>\n\nstatic int _errno_var = 0;\n\n"
    if loc:
        common += loc + "\n\n"
    if errlist:
        common += errlist.group(0).replace("static const char *const", "const char *const") + "\n\n"
    if nerr:
        common += nerr.group(0).replace("static const int", "const int") + "\n"
    write(os.path.join(ROOT, "errno/_common.c"), common)

    map_body = extract_fn(errno_src, "map_fs_error")
    write(
        os.path.join(ROOT, "errno/map_fs_error.c"),
        '#include <errno.h>\n#include "internal/fs_errors.h"\n\n' + map_body + "\n",
    )

    str_body = extract_fn(errno_src, "strerror")
    write(
        os.path.join(ROOT, "errno/strerror.c"),
        '#include <errno.h>\n#include "errno/errno_internal.h"\n\n' + str_body + "\n",
    )

    per_body = extract_fn(errno_src, "perror")
    write(
        os.path.join(ROOT, "stdio/perror.c"),
        '#include <stdio.h>\n#include <errno.h>\n\n' + per_body + "\n",
    )


def fix_time_public():
    fixes = {
        "gmtime.c": """#include <time.h>
#include "time/time_internal.h"

struct tm *gmtime(const time_t *timer)
{
    if (!timer)
        return NULL;
    fill_tm_from_time(*timer, time_gmtime_buf());
    return time_gmtime_buf();
}
""",
        "localtime.c": """#include <time.h>
#include "time/time_internal.h"

struct tm *localtime(const time_t *timer)
{
    if (!timer)
        return NULL;

    read_timezone_offset();

    time_t t = *timer + (*time_timezone_offset()) * 3600;
    fill_tm_from_time(t, time_localtime_buf());
    return time_localtime_buf();
}
""",
        "ctime.c": """#include <time.h>
#include "time/time_internal.h"

char *ctime(const time_t *timer)
{
    if (!timer)
        return NULL;
    struct tm *tm = localtime(timer);
    return asctime(tm);
}
""",
        "asctime.c": """#include <time.h>
#include "time/time_internal.h"

char *asctime(const struct tm *tm)
{
    static const char *wday_name[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon_name[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char *asctime_buf = time_asctime_buf();
    if (!tm)
        return NULL;

    int y = tm->tm_year + 1900;
    int month = tm->tm_mon;
    int day = tm->tm_mday;
    int hour = tm->tm_hour;
    int min = tm->tm_min;
    int sec = tm->tm_sec;
    int wday = tm->tm_wday;

    const char *wd = (wday >= 0 && wday < 7) ? wday_name[wday] : "Day";
    const char *mn = (month >= 0 && month < 12) ? mon_name[month] : "Mon";

    int pos = 0;
    for (const char *p = wd; *p; ++p)
        asctime_buf[pos++] = *p;
    asctime_buf[pos++] = ' ';
    for (const char *p = mn; *p; ++p)
        asctime_buf[pos++] = *p;
    asctime_buf[pos++] = ' ';
    if (day < 10)
    {
        asctime_buf[pos++] = ' ';
        asctime_buf[pos++] = '0' + day;
    }
    else
    {
        asctime_buf[pos++] = '0' + (day / 10);
        asctime_buf[pos++] = '0' + (day % 10);
    }
    asctime_buf[pos++] = ' ';
    asctime_buf[pos++] = '0' + ((hour / 10) % 10);
    asctime_buf[pos++] = '0' + (hour % 10);
    asctime_buf[pos++] = ':';
    asctime_buf[pos++] = '0' + ((min / 10) % 10);
    asctime_buf[pos++] = '0' + (min % 10);
    asctime_buf[pos++] = ':';
    asctime_buf[pos++] = '0' + ((sec / 10) % 10);
    asctime_buf[pos++] = '0' + (sec % 10);
    asctime_buf[pos++] = ' ';
    int y1000 = y / 1000, y100 = (y / 100) % 10, y10 = (y / 10) % 10, y1 = y % 10;
    asctime_buf[pos++] = '0' + y1000;
    asctime_buf[pos++] = '0' + y100;
    asctime_buf[pos++] = '0' + y10;
    asctime_buf[pos++] = '0' + y1;
    asctime_buf[pos++] = '\n';
    asctime_buf[pos] = '\0';
    return asctime_buf;
}
""",
    }
    for name, content in fixes.items():
        write(os.path.join(ROOT, "time", name), content)


def fix_public_headers():
    errno_h = os.path.join(REPO, "runtime/include/errno.h")
    with open(errno_h) as f:
        text = f.read()
    text = text.replace("#define ENODEV 19;", "#define ENODEV 19")
    write(errno_h, text)

    stdio_h = os.path.join(REPO, "runtime/include/stdio.h")
    with open(stdio_h) as f:
        text = f.read()
    if "void write(" not in text:
        text = text.replace("int fputc", "void write(const char *str);\n\nint fputc")
        write(stdio_h, text)


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
    write(
        os.path.join(ROOT, "stdlib/heap_internal.c"),
        '#include "stdlib/heap_internal.h"\n#include "internal/syscalls.h"\n'
        "#include <errno.h>\n#include <string.h>\n\n" + body,
    )


def main():
    rebuild_fs_errors()
    rebuild_time_internal()
    rebuild_errno()
    fix_time_public()
    fix_public_headers()
    print("rebuilt errno/time internal modules")


if __name__ == "__main__":
    main()
