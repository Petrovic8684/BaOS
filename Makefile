# Tools
NASM = nasm
QEMU = qemu-system-i386

# Files
BOOT_SRC = bootloader/boot.asm
BOOT_BIN = bootloader/boot.bin

# Default target
all: $(BOOT_BIN)

# Bootloader
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $< -o $@

# Runs QEMU
run: $(BOOT_BIN)
	$(QEMU) $(BOOT_BIN)

# Clean
clean:
	rm -f $(BOOT_BIN)

