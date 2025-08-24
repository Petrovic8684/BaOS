#ifndef FILLING_H
#define FILLING_H

#include "../../kernel/fs/fs.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/display/display.h"

void editor_redraw();
void editor_load_file(const char *filename);
void editor_save_file(const char *filename);
void filling_main(const char *filename);

#endif