#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_irq_handler(int irq);

void keyboard_init(void);
unsigned char read(void);

#endif
