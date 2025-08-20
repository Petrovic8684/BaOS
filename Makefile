# Tools
NASM = nasm
QEMU = qemu-system-i386
CC = i686-elf-gcc
LD = i686-elf-ld
CAT = cat

# Files
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin
KERNEL_SRC = kernel/kernel.c
KERNEL_OBJ = kernel/kernel.o
KERNEL_BIN = kernel/kernel.bin
KERNEL_LD = kernel/link.ld
IMG = baos.img

# Default target
all: $(IMG)

# Bootloader
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# Kernel
$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) -ffreestanding -m32 -c $< -o $@

# Link kernel as flat binary
$(KERNEL_BIN): $(KERNEL_OBJ) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $<

# Image
$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
	$(CAT) $(BOOT_BIN) $(KERNEL_BIN) > $@

# Run in QEMU
run: $(IMG)
	$(QEMU) -drive format=raw,file=$(IMG),if=floppy

# Clean
clean:
	rm -f $(BOOT_BIN) $(KERNEL_OBJ) $(KERNEL_BIN) $(IMG)
