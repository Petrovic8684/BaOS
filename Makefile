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
    kernel/syscalls/syscalls.c \
    kernel/system/system.c \
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

KERNEL_ASM_SRCS = kernel/system/idt/idt_flush.asm kernel/paging/page_fault.asm

USER_SRCS = applications/bao.c
USER_BINS = $(USER_SRCS:.c=.bin)

KERNEL_OBJS = $(KERNEL_SRCS:.c=.o) $(KERNEL_ASM_SRCS:.asm=.o)
KERNEL_BIN = kernel/kernel.bin
KERNEL_LD  = kernel/link.ld
IMG = baos.img
IMG_SIZE = 16

# ---------------- Runtime ----------------
RUNTIME_SRCS = runtime/runtime.c runtime/src/stdio.c runtime/src/stdlib.c
RUNTIME_OBJS = $(RUNTIME_SRCS:.c=.o)
RUNTIME_INCLUDE = -I./runtime/include

# ---------------- Default target ----------------
all: $(IMG)

# ---------------- Bootloader ----------------
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# ---------------- Kernel build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -c $< -o $@

%.o: %.asm
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS)

# ---------------- Runtime build ----------------
%.o: runtime/src/%.c
	$(CC) -ffreestanding -m32 -c $(RUNTIME_INCLUDE) $< -o $@

# ---------------- User programs build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie $(RUNTIME_INCLUDE) -c $< -o $@

# Link user program: bao.o + svi runtime obj-ovi
%.bin: %.o $(RUNTIME_OBJS)
	$(LD) -m elf_i386 -T kernel/loader/user.ld -o $@ $^

# ---------------- Disk image -----------------
$(IMG): $(BOOT_BIN) $(KERNEL_BIN) $(USER_BINS)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=$(IMG_SIZE)
	$(DD) if=$(BOOT_BIN) of=$(IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(IMG) seek=1 conv=notrunc
	for prog in $(USER_BINS); do \
	    $(PY) tools/mkfs_inject.py $(IMG) $$prog; \
	done

# ---------------- Run & Clean ----------------
run: $(IMG)
	$(QEMU) -drive format=raw,file=$(IMG),if=ide

clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG) \
	      $(USER_BINS) $(RUNTIME_OBJS)
