#include "keyboard.h"
#include "../system/cpu/isr.h"
#include "../system/shell.h"
#include <stdint.h>

static const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
};

volatile char last_key = 0;
volatile int key_waiting = 0;

void keyboard_callback(Registers* r) {
    uint8_t scancode;
    asm volatile ("inb %1, %0" : "=a"(scancode) : "dN"(0x60));
    if (!(scancode & 0x80)) {
        char c = 0;
        if (scancode < 128) c = kbd_us[scancode];
        
        if (scancode == 0x48) c = 'W';
        if (scancode == 0x50) c = 'S';
        if (scancode == 0x4B) c = 'A';
        if (scancode == 0x4D) c = 'D';

        if (c) {
            last_key = c;
            key_waiting = 1;
        }
    }
}

void keyboard_init() {
    irq_register_handler(33, keyboard_callback);
}
