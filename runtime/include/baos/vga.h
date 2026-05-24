#ifndef BAOS_VGA_H
#define BAOS_VGA_H

typedef struct
{
    int row;
    int col;
    char ch;
    unsigned char attr;
} vga_cell_t;

void vga_get_cell(int row, int col, char *ch, unsigned char *attr);
void vga_put_cell(int row, int col, char ch, unsigned char attr);

static inline unsigned char vga_invert_attr(unsigned char attr)
{
    unsigned char fg = attr & 0x0F;
    unsigned char bg = (attr >> 4) & 0x0F;
    return (unsigned char)((fg << 4) | bg);
}

#endif
