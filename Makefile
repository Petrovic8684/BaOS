# ---------------- Tools ----------------
NASM = nasm
QEMU = qemu-system-i386
CC = i686-elf-gcc
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy
DD = dd
RM = rm -f
PY = python

# ---------------- Files ----------------
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin

KERNEL_SRCS = \
    kernel/kernel.c \
    kernel/fs/fs.c \
    kernel/paging/paging.c \
    kernel/paging/page_fault_handler.c \
    kernel/loader/loader.c \
    kernel/api/syscalls.c \
    kernel/info/info.c \
    kernel/system/idt/idt.c \
    kernel/system/gdt/gdt.c \
    kernel/system/tss/tss.c \
    kernel/system/acpi/acpi.c \
    kernel/drivers/keyboard/keyboard.c \
    kernel/drivers/display/display.c \
    kernel/drivers/rtc/rtc.c \
    kernel/drivers/disk/ata.c \
    kernel/helpers/ports/ports.c \
    kernel/helpers/string/string.c \
    kernel/helpers/bcd/bcd.c \
    kernel/helpers/memory/memory.c

KERNEL_ASM_SRCS = \
    kernel/system/idt/idt_flush.asm \
    kernel/paging/page_fault.asm

SHELL_SRCS = applications/shell/shell.c
SHELL_DEPS = applications/shell/wrappers/wrappers.c
SHELL_BIN = $(SHELL_SRCS:.c=.bin)

CALC_SRC = applications/calc/calc.c
CALC_BIN = $(CALC_SRC:.c=.bin)

FILLING_SRC = applications/filling/filling.c
FILLING_BIN = $(FILLING_SRC:.c=.bin)

COMPILER_SRC = tools/baoc/baoc.c
COMPILER_BIN = $(COMPILER_SRC:.c=.bin)

INSPECT_SRC = applications/inspect.c
INSPECT_BIN = $(INSPECT_SRC:.c=.bin)

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o) $(KERNEL_ASM_SRCS:.asm=.o)
KERNEL_BIN = kernel/kernel.bin
KERNEL_LD  = kernel/link.ld

IMG = baos.img
IMG_SIZE = 16

# ---------------- Runtime ----------------
RUNTIME_START_SRC = runtime/crt0.c
RUNTIME_START_OBJ = runtime/crt0.o
RUNTIME_START_BIN = runtime/crt0.bin
LIBC_SYM = runtime/libc.sym

RUNTIME_TEST_SRC = runtime/crt0.c
RUNTIME_TEST_OBJ = runtime/crt0.o
RUNTIME_TEST_BIN = runtime/crt0.bin

RUNTIME_SRC_LIST = \
    runtime/src/stdio.c \
    runtime/src/stdlib.c \
    runtime/src/string.c \
    runtime/src/ctype.c \
    runtime/src/dirent.c \
    runtime/src/sys_stat.c \
    runtime/src/sys_utsname.c \
    runtime/src/time.c \
    runtime/src/unistd.c \
    runtime/src/math.c

RUNTIME_SRC_OBJS = $(RUNTIME_SRC_LIST:.c=.o)

RUNTIME_INCLUDE = -Iruntime/include
RUNTIME_BIN = runtime/libc.bin

# ---------------- Default target ----------------
all: $(IMG)

# ---------------- Bootloader ----------------
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# ---------------- Kernel build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie $(RUNTIME_INCLUDE) -c $< -o $@

%.o: %.asm
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS)

# ---------------- Runtime build ----------------
$(RUNTIME_START_OBJ): $(RUNTIME_START_SRC)
	$(CC) -ffreestanding -m32 -c $(RUNTIME_INCLUDE) $< -o $@

$(RUNTIME_START_BIN): $(RUNTIME_START_OBJ)
	$(OBJCOPY) -O binary $< $@

$(RUNTIME_BIN): $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 --oformat binary -o $@ $^

$(LIBC_SYM): $(RUNTIME_BIN)
	$(PY) tools/generate_sym.py runtime/libc.bin > $@

# ---------------- Shell & Apps build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie $(RUNTIME_INCLUDE) -c $< -o $@

%.bin: %.o $(SHELL_DEPS:.c=.o) $(RUNTIME_START_OBJ) $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 -T kernel/loader/user.ld -o $@ $^

%.bin: %.o $(RUNTIME_START_OBJ) $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 -T kernel/loader/user.ld -o $@ $^

# ---------------- Disk image -----------------
$(IMG): $(BOOT_BIN) $(KERNEL_BIN) $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(INSPECT_BIN) \
       $(RUNTIME_BIN) $(RUNTIME_START_BIN) $(LIBC_SYM)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=$(IMG_SIZE)
	$(DD) if=$(BOOT_BIN) of=$(IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(IMG) seek=1 conv=notrunc

	for h in runtime/include/*.h; do \
		$(PY) tools/mkfs_inject.py $(IMG) $$h /lib/include; \
	done

	$(PY) tools/mkfs_inject.py $(IMG) $(RUNTIME_START_BIN) /lib

	$(PY) tools/mkfs_inject.py $(IMG) $(RUNTIME_BIN) /lib

	$(PY) tools/mkfs_inject.py $(IMG) $(LIBC_SYM) /lib

	$(PY) tools/mkfs_inject.py $(IMG) applications/bao.c /programs

	for prog in $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(INSPECT_BIN); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$prog /programs; \
	done

# ---------------- Run & Clean ----------------
run: $(IMG)
	$(QEMU) -drive format=raw,file=$(IMG),if=ide

clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG) \
	      $(SHELL_BIN) $(SHELL_DEPS:.c=.o) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(LIBC_SYM) \
	      $(RUNTIME_SRC_OBJS) $(RUNTIME_START_OBJ) $(RUNTIME_START_BIN) $(RUNTIME_BIN) $(INSPECT_BIN)
