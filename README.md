# BaOS (Bourne again Operating System)

<img src="assets/bao.png" alt="bao" width="100px">

**BaOS** is a simple **x86 operating system** for managing **directories** and **text files**.
Currently, it includes a **custom bootloader** written in assembly.

In the future, BaOS will support the **FAT12 file system**, and the bootloader will switch to **protected mode** before jumping to a kernel written in **C**. ⚡

The long-term goal is to add a custom, simple Unix-like shell with commands such as: `list`, `changedir`, `makedir`, `deletedir`, `makefile`, `readfile`, and `deletefile`.

## Getting started

### Requirements

- **[NASM](https://www.nasm.us/)** – Netwide Assembler
- **[QEMU](https://www.qemu.org/)** – Quick EMUlator
- **[Make](https://www.gnu.org/software/make/)** – GNU Build tool

### Build steps

1. **Build the bootloader:** Open a terminal in the project directory and run:

```bash
make
```

2. **Start the OS in QEMU:** After building, launch the OS with:

```bash
make run
```

After running this command, QEMU will launch your BaOS bootloader.

## Credits 🙏

- 🌐 <a href="https://www.flaticon.com/free-icons/xiao-long-bao" title="xiao long bao icons">Xiao long bao icons created by zero_wing - Flaticon</a>

_Happy coding!_ 🚀
