#!/usr/bin/env python3
"""Generate /lib/baoc_syms for baoc linker (libc function offsets in libc.bin)."""
import glob
import os
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNTIME_BIN = os.path.join(ROOT, "runtime", "libc.bin")
RUNTIME_LIB = os.path.join(ROOT, "runtime", "libc.a")
OUT_DAT = os.path.join(ROOT, "runtime", "baoc_syms.dat")
REF_ELF = os.path.join(ROOT, "tools", "_baoc_libc_ref.elf")
MAGIC = b"BSYM"
LD = os.environ.get("BAOS_LD", "i686-elf-ld")
NM = os.environ.get("BAOS_NM", "i686-elf-nm")


def run(cmd):
    try:
        return subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(e.output or "")
        raise


def runtime_objs():
    pattern = os.path.join(ROOT, "runtime", "src", "**", "*.o")
    return sorted(glob.glob(pattern, recursive=True))


def write_empty():
    with open(OUT_DAT, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<II", 0, 0))


def main():
    objs = runtime_objs()
    if not objs:
        print("gen_baoc_syms: no runtime .o files; run make runtime/libc.a first")
        write_empty()
        return 0

    if not os.path.isfile(RUNTIME_BIN):
        print("gen_baoc_syms: missing %s; build runtime first" % RUNTIME_BIN)
        if not os.path.isfile(OUT_DAT):
            write_empty()
        return 0

    libc_sz = os.path.getsize(RUNTIME_BIN)

    if not objs:
        write_empty()
        return 0

    run([LD, "-m", "elf_i386", "-o", REF_ELF] + objs)

    nm_out = run([NM, REF_ELF])
    syms = []
    for line in nm_out.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        addr_s, sym_type, name = parts[0], parts[1], parts[2]
        if sym_type not in ("T", "t", "W", "w"):
            continue
        if name.startswith("."):
            continue
        try:
            off = int(addr_s, 16)
        except ValueError:
            continue
        if off >= libc_sz:
            continue
        syms.append((name, off))

    syms.sort(key=lambda x: x[0])
    seen = set()
    unique = []
    for name, off in syms:
        if name in seen:
            continue
        seen.add(name)
        unique.append((name, off))

    with open(OUT_DAT, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<II", libc_sz, len(unique)))
        for name, off in unique:
            nb = name.encode("ascii")
            if len(nb) > 255:
                nb = nb[:255]
            f.write(struct.pack("<B", len(nb)))
            f.write(nb)
            f.write(struct.pack("<I", off))

    print("gen_baoc_syms: wrote %s (%u symbols, libc %u bytes)" % (OUT_DAT, len(unique), libc_sz))
    return 0


if __name__ == "__main__":
    sys.exit(main())
