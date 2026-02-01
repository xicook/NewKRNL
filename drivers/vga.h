#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void vga_init();
void vga_putchar(char c);
void vga_puts(const char* s);
void vga_clear();
void vga_set_color(uint8_t fg, uint8_t bg);

#endif
