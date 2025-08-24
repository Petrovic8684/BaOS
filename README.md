<img src="assets/bao.png" alt="bao" width="100px">

# BaOS (Bourne again Operating System)

**BaOS** is a simple **x86 operating system** for managing **directories** and **text files**. It currently includes a **custom bootloader** written in assembly and a **C kernel** that runs a basic **shell** for file and directory management.

The kernel initializes a root directory `/` and supports storage with up to **32 directories** and **64 files**. **Text input and output** are handled via **VGA**, with scrolling and a simple prompt.

Thanks to a custom written **ATA PIO driver**, the entire **file system** is now **persistent on disk** across reboots, instead of being stored only in memory. The kernel uses a **custom file system** implementation for managing files and directories.

The bootloader loads the **kernel** from disk in real mode and then switches the CPU to **protected mode** before jumping to the C kernel, ensuring a **flat memory model**.

Future plans for BaOS include moving the **shell** to **ring 3**, integrating **user programs** like the **text editor "Filling"** and **calculator** (currently still part of the shell), and eventually developing a **graphical user interface (GUI)**.

## General architecture 🏛️

BaOS is structured in a clear hierarchical architecture that separates hardware management from user interaction. At the lowest level is the hardware, including the CPU, memory, and I/O devices. The kernel sits directly on top of the hardware and is responsible for all low-level management, including device I/O and the file system. It exposes its functionality to higher-level components through a set of C functions, which serve as a stable API.

<div align="center">
    <img src="assets/architecture.svg" alt="architecture">
</div>
</br>

The shell operates at the top of this hierarchy. It interacts with the user, parses input, and determines which kernel functions to invoke. The shell itself does not have direct access to the hardware and relies entirely on the kernel’s API to perform operations like reading/writing files, creating directories, or displaying system information. This separation ensures a clean modular design, where the kernel handles all hardware-specific logic and the shell focuses on user interaction and command processing.

## Getting started 🥟

### Requirements ⚠️

- **[NASM](https://www.nasm.us/)** – Netwide Assembler
- **[QEMU](https://www.qemu.org/)** – Quick EMUlator
- **[Make](https://www.gnu.org/software/make/)** – GNU Build tool
- **[i686-elf-gcc](https://github.com/lordmilko/i686-elf-tools/releases)** - Cross compiler
- **[i686-elf-ld](https://github.com/lordmilko/i686-elf-tools/releases)** - Cross linker

### Build steps 🛠️

1. **Build the project:** Open a terminal in the project directory and run:

```bash
make
```

2. **Start the OS in QEMU:** After building, launch the OS with:

```bash
make run
```

After running this command, QEMU will launch your BaOS bootloader. 🏁

## Credits 🙏

- 🌐 <a href="https://www.flaticon.com/free-icons/xiao-long-bao" title="xiao long bao icons">Xiao long bao icons created by zero_wing - Flaticon</a>

_Happy coding!_ 🚀
