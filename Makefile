# ---------------- Tools ----------------
NASM    = nasm
QEMU    = qemu-system-i386
CC      = i686-elf-gcc
LD      = i686-elf-ld
OBJCOPY = i686-elf-objcopy
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
UTILS_OBJS	 = $(UTILS_SRCS:.c=.o)

CALC_SRC     = applications/calc/calc.c
CALC_BIN     = applications/calc/calc.bin

FILLING_SRC  = applications/filling/filling.c
FILLING_BIN  = applications/filling/filling.bin

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

RUNTIME_SRC_LIST = $(wildcard runtime/src/*.c)

RUNTIME_SRC_OBJS = $(RUNTIME_SRC_LIST:.c=.o)
RUNTIME_INCLUDE  = -Iruntime/include
RUNTIME_BIN      = runtime/libc.bin
RUNTIME_LIB      = runtime/libc.a

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

$(RUNTIME_LIB): $(RUNTIME_SRC_OBJS)
	ar rcs $@ $^

# ---------------- Shell & Apps build ----------------
%.o: %.c
	$(CC) -ffreestanding -m32 -nostdlib -fno-pie $(RUNTIME_INCLUDE) -c $< -o $@

%.bin: %.o $(CRT0_OBJ) $(RUNTIME_LIB)
	$(LD) -m elf_i386 -T kernel/loader/user.ld --gc-sections -o $@ $^

applications/shell/utils/dirlist.bin: applications/shell/utils/dirlist.o $(DIRLIST_COMMON_OBJ) $(CRT0_OBJ) $(RUNTIME_LIB)
	$(LD) -m elf_i386 -T kernel/loader/user.ld --gc-sections -o $@ $^

applications/shell/utils/mousedirlist.bin: applications/shell/utils/mousedirlist.o $(DIRLIST_COMMON_OBJ) $(CRT0_OBJ) $(RUNTIME_LIB)
	$(LD) -m elf_i386 -T kernel/loader/user.ld --gc-sections -o $@ $^

# ---------------- Disk image -----------------
$(IMG): $(BOOT_BIN) $(KERNEL_BIN) $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(UTILS_BIN) \
       $(RUNTIME_BIN) $(CRT0_BIN)
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

# ---------------- Run & Clean ----------------
run: $(IMG)
	$(QEMU) -m 3G -drive format=raw,file=$(IMG),if=ide -serial stdio -vnc :0 \
	#	-audiodev pa,id=snd0 \
	#	-machine pcspk-audiodev=snd0 \
	#	-device intel-hda

clean:
	$(RM) $(BOOT_BIN) $(KERNEL_OBJS) $(KERNEL_BIN) $(IMG) \
	      $(SHELL_BIN) $(CALC_BIN) $(FILLING_BIN) $(UTILS_BIN) $(UTILS_OBJS) $(DIRLIST_COMMON_OBJ) \
	      $(RUNTIME_SRC_OBJS) $(CRT0_OBJ) $(CRT0_BIN) $(RUNTIME_BIN) $(RUNTIME_LIB) $(USER_OBJS)
