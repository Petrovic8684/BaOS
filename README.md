<img src="github/bao.png" alt="bao" width="100px">

# BaOS (Bourne again Operating System)

**BaOS** is a simple **x86 32 bit operating system**. It includes a custom bootloader written in assembly and a C kernel that runs a basic shell for file and directory management.

The kernel implements **paging** with **identity mapping**, which allows it to map virtual memory directly to physical memory for simplicity while still enforcing access restrictions. Kernel memory is protected with user/supervisor bits, ensuring that programs running in **ring 3** cannot accidentally or maliciously overwrite critical kernel data. This creates a safe boundary between kernel space and user space, providing the foundation for running untrusted user programs.

All drivers in BaOS are implemented as interrupt-based, ensuring efficient and responsive handling of hardware events without relying on polling. This approach allows the system to remain reactive even when multiple programs access different hardware resources simultaneously. A custom ATA PIO driver ensures the file system is **persistent across reboots**, rather than being stored only in memory.

A custom **ELF loader** manages loading user programs into memory and performs a lightweight context switch, mainly adjusting stack-related values to transfer control to user space. All communication between user programs and the kernel happens through a **syscall API**, keeping the kernel isolated from direct user-level memory access.

BaOS comes with a **custom C runtime**, including `crt0` and supporting runtime libraries. It provides most of the standard ANSI C functionality and parts of some POSIX headers, allowing programs to rely on familiar C constructs while using custom syscalls under the hood. The system currently includes a **shell**, a **text editor** called `filling`, a **calculator** called `calc` capable of evaluating numerical expressions as well as linear and general equations using the Newton-Raphson method.

Looking forward, one of the main goals is to eventually port a **C compiler** to BaOS once the runtime environment becomes mature enough, enabling full compilation of user programs directly on the system.

BaOS is also planned to evolve into a more fully-featured OS, including a **graphical user interface (GUI), sound support, multitasking**, and the ability to handle common file types such as images and audio. Additionally, a **TCP/IP stack** is planned to enable networking features such as sending and receiving emails, making HTTP requests, and other network communications. Based on this networking layer, a package manager will allow the BaOS community to share and distribute programs.

These improvements aim to make BaOS a versatile, minimal, and safe operating system capable of running multiple user programs concurrently with persistent storage and a rich set of multimedia, UI, and networking capabilities.

## General architecture 🏛️

BaOS is structured in a clear hierarchical architecture that separates hardware management from user interaction. At the lowest level is the hardware, including the CPU, memory, and I/O devices. The kernel sits directly on top of the hardware and is responsible for all low-level management, including device I/O and the file system. It exposes its functionality to higher-level components through a set of C functions, which serve as a stable API.

<div align="center">
    <img src="github/architecture.svg" alt="architecture">
</div>
</br>

The shell and user applications operate at the top of this hierarchy in **ring 3**, ensuring that they cannot access kernel memory or hardware directly. These programs use a custom **libc** runtime, which provides standard C functionality while translating requests into system calls that request services from the kernel. This layer allows user programs to perform operations such as reading and writing files, creating directories, or interacting with other system services, without breaking the separation between user space and kernel space. By keeping the shell and all user applications in ring 3, BaOS enforces security and stability while providing a flexible environment for program execution.

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
