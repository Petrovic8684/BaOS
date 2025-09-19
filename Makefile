# ---------------- Tools ----------------
NASM    = nasm
QEMU    = qemu-system-i386
CC      = i686-elf-gcc
LD      = i686-elf-ld
OBJCOPY = i686-elf-objcopy
DD      = dd
RM      = rm -f
PY      = python

# ---------------- Files ----------------
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin

KERNEL_SRCS = \
	kernel/kernel.c \
	kernel/fs/fs.c \
	kernel/paging/paging.c \
	kernel/loader/loader.c \
	kernel/api/syscalls.c \
	kernel/info/info.c \
	kernel/system/pic/pic.c \
	kernel/system/idt/idt.c \
	kernel/system/idt/isr/isr_handlers.c \
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
	kernel/system/idt/irq/irq_stubs.asm \
	kernel/system/idt/isr/isr_stubs.asm \
	

SHELL_SRCS   = applications/shell/shell.c
SHELL_DEPS 	 = applications/shell/utils/common/fs_common.c
SHELL_BIN    = applications/shell/shell.bin

UTILS_SRCS   =  applications/shell/utils/changedir.c \
			 	applications/shell/utils/clear.c \
				applications/shell/utils/copy.c \
				applications/shell/utils/date.c \
				applications/shell/utils/deletedir.c \
				applications/shell/utils/deletefile.c \
				applications/shell/utils/echo.c \
				applications/shell/utils/help.c \
				applications/shell/utils/list.c \
				applications/shell/utils/makedir.c \
				applications/shell/utils/makefile.c \
				applications/shell/utils/move.c \
				applications/shell/utils/osname.c \
				applications/shell/utils/readfile.c \
				applications/shell/utils/shutdown.c \
				applications/shell/utils/restart.c \
				applications/shell/utils/version.c \
				applications/shell/utils/where.c \
				applications/shell/utils/writefile.c \
				applications/shell/utils/whatis.c \

UTILS_BIN    = $(UTILS_SRCS:.c=.bin)
UTILS_OBJS	 = $(UTILS_SRCS:.c=.o)

CALC_SRC     = applications/calc/calc.c
CALC_BIN     = applications/calc/calc.bin

FILLING_SRC  = applications/filling/filling.c
FILLING_BIN  = applications/filling/filling.bin

COMPILER_BIN = applications/baoc/baoc.bin

TEST_SRC	 = applications/test.c
TEST_BIN	 = applications/test.bin

USER_OBJS    = applications/*/*.o

DOCS = applications/shell/utils/docs/*

KERNEL_OBJS  = $(KERNEL_SRCS:.c=.o) $(KERNEL_ASM_SRCS:.asm=.o)
KERNEL_BIN   = kernel/kernel.bin
KERNEL_LD    = kernel/link.ld

IMG       = baos.img
IMG_SIZE  = 16

# ---------------- Runtime ----------------
CRT0_SRC  = runtime/crt0.c
CRT0_OBJ  = runtime/crt0.o
CRT0_BIN  = runtime/crt0.bin

LIBC_SYM  = runtime/libc.sym

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
RUNTIME_INCLUDE  = -Iruntime/include
RUNTIME_BIN      = runtime/libc.bin

# ---------------- Default target ----------------
all: $(IMG)

# ---------------- Bootloader ----------------
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# ---------------- Kernel build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie -c $< -o $@

%.o: %.asm
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS)

# ---------------- Runtime build ----------------
$(CRT0_OBJ): $(CRT0_SRC)
	$(CC) -ffreestanding -m32 -c $(RUNTIME_INCLUDE) $< -o $@

$(CRT0_BIN): $(CRT0_OBJ)
	$(OBJCOPY) -O binary $< $@

$(RUNTIME_BIN): $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 --oformat binary -o $@ $^

$(LIBC_SYM): $(RUNTIME_BIN)
	$(PY) tools/generate_sym.py $(RUNTIME_BIN) > $@

# ---------------- Shell & Apps build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie $(RUNTIME_INCLUDE) -c $< -o $@

%.bin: %.o $(SHELL_DEPS:.c=.o) $(CRT0_OBJ) $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 -T kernel/loader/user.ld -o $@ $^

%.bin: %.o $(CRT0_OBJ) $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 -T kernel/loader/user.ld -o $@ $^

# ---------------- Disk image -----------------
$(IMG): $(BOOT_BIN) $(KERNEL_BIN) $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(TEST_BIN) $(UTILS_BIN) \
       $(RUNTIME_BIN) $(CRT0_BIN) $(LIBC_SYM)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=$(IMG_SIZE)
	$(DD) if=$(BOOT_BIN) of=$(IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(IMG) seek=1 conv=notrunc

	for h in runtime/include/*.h; do \
		$(PY) tools/mkfs_inject.py $(IMG) $$h /lib/include; \
	done

	$(PY) tools/mkfs_inject.py $(IMG) $(CRT0_BIN) /lib
	$(PY) tools/mkfs_inject.py $(IMG) $(RUNTIME_BIN) /lib
	$(PY) tools/mkfs_inject.py $(IMG) $(LIBC_SYM) /lib

	for prog in $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(TEST_BIN); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$prog /programs; \
	done

	for prog in $(UTILS_BIN); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$prog /programs/utils; \
	done

	for doc in $(DOCS); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$doc /docs; \
	done

# ---------------- Run & Clean ----------------
run: $(IMG)
	$(QEMU) -m 4G -drive format=raw,file=$(IMG),if=ide

clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG) \
	      $(SHELL_BIN) $(SHELL_DEPS:.c=.o) $(CALC_BIN) $(FILLING_BIN) $(COMPILER_BIN) $(TEST_BIN) $(UTILS_BIN) $(UTILS_OBJS) $(LIBC_SYM) \
	      $(RUNTIME_SRC_OBJS) $(CRT0_OBJ) $(CRT0_BIN) $(RUNTIME_BIN) $(USER_OBJS)
