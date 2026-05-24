#include <baos/vga.h>

#define SYS_VGA_GET_CELL 34
#define SYS_VGA_PUT_CELL 35

static inline int sys_vga_get_cell(vga_cell_t *cell)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_VGA_GET_CELL), [arg] "r"(cell)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_vga_put_cell(vga_cell_t *cell)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_VGA_PUT_CELL), [arg] "r"(cell)
        : "eax", "ebx", "memory");
    return (int)ret;
}

void vga_get_cell(int row, int col, char *ch, unsigned char *attr)
{
    vga_cell_t cell;
    cell.row = row;
    cell.col = col;
    cell.ch = ' ';
    cell.attr = 0x07;
    sys_vga_get_cell(&cell);
    if (ch)
        *ch = cell.ch;
    if (attr)
        *attr = cell.attr;
}

void vga_put_cell(int row, int col, char ch, unsigned char attr)
{
    vga_cell_t cell;
    cell.row = row;
    cell.col = col;
    cell.ch = ch;
    cell.attr = attr;
    sys_vga_put_cell(&cell);
}
