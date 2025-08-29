#ifndef LOADER_H
#define LOADER_H

#include "../drivers/display/display.h"
#include "../fs/fs.h"
#include "../paging/paging.h"
#include "../system/tss/tss.h"

#define PT_LOAD 1
#define ELF_MAGIC 0x7F454C46
#define USER_LOAD_ADDR 0x20000

#define USER_STACK_TOP 0x200000
#define USER_STACK_PAGES 4

typedef unsigned int Elf32_Addr;
typedef unsigned int Elf32_Off;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Word;

typedef struct
{
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

extern unsigned int loader_return_eip;
extern unsigned int loader_saved_esp;
extern unsigned int loader_saved_ebp;

extern void (*loader_post_return_callback)(void);

typedef void (*user_entry_t)(void);
void load_next_program(void);
void load_user_program(const char *name, const char **user_argv);

#endif