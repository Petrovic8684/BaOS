import os
import subprocess

RUNTIME_OBJS = [
    "runtime/src/ctype.o",
    "runtime/src/dirent.o",
    "runtime/src/math.o",
    "runtime/src/stdio.o",
    "runtime/src/stdlib.o",
    "runtime/src/string.o",
    "runtime/src/sys_stat.o",
    "runtime/src/sys_utsname.o",
    "runtime/src/time.o",
    "runtime/src/unistd.o",
]

LIBC_BIN = "runtime/libc.bin"
LIBC_SYM = "runtime/libc.sym"

def generate_libc_sym():
    with open(LIBC_SYM, "w") as symf:
        offset = 0
        for obj in RUNTIME_OBJS:
            nm_output = subprocess.check_output(["nm", "--defined-only", obj]).decode()
            for line in nm_output.splitlines():
                parts = line.strip().split()
                if len(parts) >= 3:
                    sym_name = parts[2]
                    symf.write(f"{sym_name} {offset}\n")
            with open(obj, "rb") as f:
                obj_data = f.read()
                offset += len(obj_data)
    print(f"Generated {LIBC_SYM}")

if __name__ == "__main__":
    generate_libc_sym()
