# Tools
NASM = nasm
QEMU = qemu-system-i386
CC = i686-elf-gcc
LD = i686-elf-ld
DD = dd
RM = rm -f

# Files
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin

KERNEL_SRCS = \
    kernel/kernel.c \
    kernel/fs/fs.c \
    kernel/system/system.c \
    kernel/system/acpi/acpi.c \
    kernel/drivers/keyboard/keyboard.c \
    kernel/drivers/display/display.c \
    kernel/drivers/rtc/rtc.c \
    kernel/drivers/disk/ata.c \
    kernel/helpers/ports/ports.c \
    kernel/helpers/string/string.c \
    kernel/helpers/bcd/bcd.c \
    kernel/helpers/memory/memory.c

SHELL_SRCS = \
    shell/shell.c \
    shell/history/history.c \
    shell/wrappers/wrappers.c
    
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
SHELL_OBJS = $(SHELL_SRCS:.c=.o)
KERNEL_BIN = kernel/kernel.bin
KERNEL_LD = kernel/link.ld
IMG = baos.img

# Image size in MB
IMG_SIZE = 16

# Default target
all: $(IMG)

# Bootloader
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# Compile kernel C files
kernel/%.o: kernel/%.c
	$(CC) -ffreestanding -m32 -c $< -o $@

# Compile shell C files
shell/%.o: shell/%.c
	$(CC) -ffreestanding -m32 -c $< -o $@

# Link kernel + shell into flat binary
$(KERNEL_BIN): $(KERNEL_OBJS) $(SHELL_OBJS) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) --oformat binary -o $@ $(KERNEL_OBJS) $(SHELL_OBJS)

# Create empty image of IMG_SIZE MB
$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
	$(DD) if=/dev/zero of=$(IMG) bs=1M count=$(IMG_SIZE)
	$(DD) if=$(BOOT_BIN) of=$(IMG) conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$(IMG) seek=1 conv=notrunc

# Run in QEMU
run: $(IMG)
	$(QEMU) -drive format=raw,file=$(IMG),if=ide

# Clean
clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(SHELL_OBJS) $(KERNEL_BIN) $(IMG)
