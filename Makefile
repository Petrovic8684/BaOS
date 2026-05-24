# ---------------- Tools ----------------
NASM    = nasm
QEMU    = qemu-system-i386
CC      = i686-elf-gcc
LD      = i686-elf-ld
OBJCOPY = i686-elf-objcopy
SIZE    = i686-elf-size
NM      = i686-elf-nm
DD      = dd
RM      = rm -f
PY      = python3

# ---------------- Files ----------------
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin

KERNEL_SRCS = \
	kernel/kernel.c \
	kernel/fs/fs.c \
	kernel/paging/paging.c \
	kernel/paging/heap/heap.c \
	kernel/drivers/drivers.c \
	kernel/loader/loader.c \
	kernel/api/syscalls.c \
	kernel/info/sys/sys.c \
	kernel/system/pic/pic.c \
	kernel/system/idt/idt.c \
	kernel/system/idt/isr/isr_handlers.c \
	kernel/system/gdt/gdt.c \
	kernel/system/tss/tss.c \
	kernel/system/acpi/acpi.c \
	kernel/drivers/pit/pit.c \
	kernel/drivers/keyboard/keyboard.c \
	kernel/drivers/display/display.c \
	kernel/drivers/rtc/rtc.c \
	kernel/drivers/disk/ata.c \
	kernel/drivers/speaker/speaker.c \
	kernel/drivers/serial/serial.c \
	kernel/drivers/mouse/mouse.c \
	kernel/drivers/speaker/melodies/melodies.c \
	kernel/helpers/ports/ports.c \
	kernel/helpers/string/string.c \
	kernel/helpers/memory/memory.c \

KERNEL_ASM_SRCS = \
	kernel/system/idt/idt_flush.asm \
	kernel/system/idt/irq/irq_stubs.asm \
	kernel/system/idt/isr/isr_stubs.asm \

SHELL_SRCS   = applications/shell/shell.c
SHELL_BIN    = applications/shell/shell.bin

UTILS_SRCS   = $(filter-out applications/shell/utils/dirlist_common.c, $(wildcard applications/shell/utils/*.c))

DIRLIST_COMMON_OBJ = applications/shell/utils/dirlist_common.o

UTILS_BIN    = $(UTILS_SRCS:.c=.bin)
UTILS_OBJS   = $(UTILS_SRCS:.c=.o)

CALC_SRC     = applications/calc/calc.c
CALC_BIN     = applications/calc/calc.bin

FILLING_SRC  = applications/filling/filling.c
FILLING_BIN  = applications/filling/filling.bin

USER_BINS    = $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(UTILS_BIN)

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

RUNTIME_SRC_LIST = $(shell find runtime/src -name '*.c' | sort)
RUNTIME_SRC_OBJS = $(RUNTIME_SRC_LIST:.c=.o)
RUNTIME_INCLUDE  = -Iruntime/include -Iruntime/src
RUNTIME_BIN      = runtime/libc.bin
RUNTIME_LIB      = runtime/libc.a

# ---------------- Compile flags ----------------
KERNEL_CFLAGS = -ffreestanding -m32 -c
USER_CFLAGS   = -ffreestanding -m32 -nostdlib -fno-pie \
                -ffunction-sections -fdata-sections \
                $(RUNTIME_INCLUDE) -c
USER_LDFLAGS  = -m32 -nostdlib -fno-pie -T kernel/loader/user.ld -Wl,--gc-sections
USER_LTO_CFLAGS  = $(USER_CFLAGS) -flto
USER_LTO_LDFLAGS = $(USER_LDFLAGS) -flto

# ---------------- Default target ----------------
all: $(IMG)

# ---------------- Bootloader ----------------
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# ---------------- Kernel build ----------------
kernel/%.o: kernel/%.c
	$(CC) $(KERNEL_CFLAGS) $< -o $@

%.o: %.asm
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS)

# ---------------- Runtime build ----------------
$(CRT0_OBJ): $(CRT0_SRC)
	$(CC) $(USER_CFLAGS) $< -o $@

runtime/src/%.o: runtime/src/%.c
	$(CC) $(USER_LTO_CFLAGS) $< -o $@

$(CRT0_BIN): $(CRT0_OBJ)
	$(OBJCOPY) -O binary $< $@

$(RUNTIME_BIN): $(RUNTIME_SRC_OBJS)
	$(LD) -m elf_i386 --oformat binary -o $@ $^

$(RUNTIME_LIB): $(RUNTIME_SRC_OBJS)
	ar rcs $@ $^

# ---------------- Shell & Apps build ----------------
applications/%.o: applications/%.c
	$(CC) $(USER_LTO_CFLAGS) $< -o $@

%.bin: %.o $(CRT0_OBJ) $(RUNTIME_LIB)
	$(CC) $(USER_LTO_LDFLAGS) -o $@ $^

applications/shell/utils/dirlist.bin: applications/shell/utils/dirlist.o $(DIRLIST_COMMON_OBJ) $(CRT0_OBJ) $(RUNTIME_LIB)
	$(CC) $(USER_LTO_LDFLAGS) -o $@ $^

applications/shell/utils/mousedirlist.bin: applications/shell/utils/mousedirlist.o $(DIRLIST_COMMON_OBJ) $(CRT0_OBJ) $(RUNTIME_LIB)
	$(CC) $(USER_LTO_LDFLAGS) -o $@ $^

# ---------------- Disk image -----------------
$(IMG): $(BOOT_BIN) $(KERNEL_BIN) $(USER_BINS) $(RUNTIME_BIN) $(CRT0_BIN)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=$(IMG_SIZE)
	$(DD) if=$(BOOT_BIN) of=$(IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(IMG) seek=1 conv=notrunc

	for prog in $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$prog /programs; \
	done

	$(PY) tools/mkfs_inject.py $(IMG) kernel/drivers/rtc/timezone /config;

	for prog in $(UTILS_BIN); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$prog /programs/utils; \
	done

	for doc in $(DOCS); do \
		$(PY) tools/mkfs_inject.py $(IMG) $$doc /docs; \
	done

# ---------------- Sizes ----------------
SAMPLE_BINS = applications/shell/utils/clear.bin \
              applications/shell/utils/echo.bin \
              applications/calc/calc.bin \
              applications/shell/shell.bin \
              applications/shell/utils/mousedirlist.bin

sizes: $(USER_BINS)
	@echo "Program sizes (text+data+bss):"
	@for prog in $(USER_BINS); do \
		printf "  %-45s " "$$prog"; \
		$(SIZE) -t $$prog 2>/dev/null | tail -1; \
	done

nm-check: $(SAMPLE_BINS)
	@echo "Sample symbol check (clear/echo should omit fmt_f and math):"
	@for prog in $(SAMPLE_BINS); do \
		echo "== $$prog =="; \
		$(NM) $$prog 2>/dev/null | grep -E ' vfprintf_fmt_| (sin|cos|pow|sqrt|malloc)$$' || true; \
		echo; \
	done

# ---------------- Run & Clean ----------------
run: $(IMG)
	$(QEMU) -m 3G -drive format=raw,file=$(IMG),if=ide -serial stdio -vnc :0

clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG) \
	      $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(UTILS_BIN) $(UTILS_OBJS) \
	      $(DIRLIST_COMMON_OBJ) applications/shell/shell.o applications/calc/calc.o \
	      applications/filling/filling.o \
	      $(RUNTIME_SRC_OBJS) $(CRT0_OBJ) $(CRT0_BIN) $(RUNTIME_BIN) $(RUNTIME_LIB)
