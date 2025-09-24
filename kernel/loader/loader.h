#ifndef LOADER_H
#define LOADER_H

#define MAX_ARGC 64
#define MAX_ARGV_LEN 128

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

typedef void (*user_entry_t)(void);

extern void (*loader_post_return_callback)(void);

void reset_loader_context(void);
void set_next_program(const char **argv);
void load_next_program(void);
void load_shell(void);
__attribute__((naked)) void return_to_loader(void);

#endif