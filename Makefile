# Tools
NASM = nasm
QEMU = qemu-system-i386
CC = i686-elf-gcc
LD = i686-elf-ld
CAT = cat
RM = rm -f

# Files
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin
KERNEL_SRCS = kernel/kernel.c kernel/IO.c
KERNEL_OBJS = kernel/kernel.o kernel/IO.o
KERNEL_BIN = kernel/kernel.bin
KERNEL_LD = kernel/link.ld
IMG = baos.img

# Default target
all: $(IMG)

# Bootloader
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# Kernel: compile all C sources
kernel/%.o: kernel/%.c
	$(CC) -ffreestanding -m32 -c $< -o $@

# Link kernel as flat binary
$(KERNEL_BIN): $(KERNEL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS)

# Image
$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
	$(CAT) $(BOOT_BIN) $(KERNEL_BIN) > $@

# Run in QEMU
run: $(IMG)
	$(QEMU) -drive format=raw,file=$(IMG),if=floppy

# Clean
clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG)
