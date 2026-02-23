#ifndef VGA_GRAPHICS_H
#define VGA_GRAPHICS_H

#include <stdint.h>

void vga_set_mode13h();
void vga_set_text_mode();
void vga_save_font();
void vga_plot_pixel(int x, int y, uint8_t color);
void vga_clear_graphics(uint8_t color);

#endif
