#include "vga.h"

static uint16_t* vga_buffer = (uint16_t*)0xB8000;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t color = 0x07; // Light Gray on Black

static void vga_update_cursor() {
    uint16_t pos = cursor_y * 80 + cursor_x;
    asm volatile ("outb %1, %0" : : "dN"(0x3D4), "a"((uint8_t)0x0F));
    asm volatile ("outb %1, %0" : : "dN"(0x3D5), "a"((uint8_t)(pos & 0xFF)));
    asm volatile ("outb %1, %0" : : "dN"(0x3D4), "a"((uint8_t)0x0E));
    asm volatile ("outb %1, %0" : : "dN"(0x3D5), "a"((uint8_t)((pos >> 8) & 0xFF)));
}

void vga_init() {
    vga_clear();
    vga_update_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    color = (bg << 4) | (fg & 0x0F);
}

void vga_clear() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t)color << 8 | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

static void vga_scroll() {
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 80; x++) {
            vga_buffer[y * 80 + x] = vga_buffer[(y + 1) * 80 + x];
        }
    }
    for (int x = 0; x < 80; x++) {
        vga_buffer[24 * 80 + x] = (uint16_t)color << 8 | ' ';
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t)color << 8 | ' ';
        }
    } else {
        vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t)color << 8 | c;
        cursor_x++;
    }

    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= 25) {
        vga_scroll();
        cursor_y = 24;
    }
    vga_update_cursor();
}

void vga_puts(const char* s) {
    while (*s) vga_putchar(*s++);
}
