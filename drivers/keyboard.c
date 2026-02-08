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

static const char kbd_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '\"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
};

static const char kbd_br[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '\'', '[', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'ç',	/* 39 */
  '~', ']',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', ';',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
};

static const char kbd_br_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '¨', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '\"', '{', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'Ç',	/* 39 */
  '^', '}',   0,		/* Left shift */
  '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', ':',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
};

volatile char last_key = 0;
volatile int key_waiting = 0;
static int shift_pressed = 0;
static int current_layout = 0; // 0=US, 1=BR

void keyboard_set_layout(int layout) {
    current_layout = layout;
}

void keyboard_callback(Registers* r) {
    uint8_t scancode;
    asm volatile ("inb %1, %0" : "=a"(scancode) : "dN"(0x60));

    if (scancode == 0x2A || scancode == 0x36) { // Shift pressed
        shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) { // Shift released
        shift_pressed = 0;
        return;
    }

    if (!(scancode & 0x80)) {
        char c = 0;
        if (scancode < 128) {
            if (current_layout == 0) {
                c = shift_pressed ? kbd_us_shift[scancode] : kbd_us[scancode];
            } else {
                c = shift_pressed ? kbd_br_shift[scancode] : kbd_br[scancode];
                // Fix slash/question mark on some ABNT2 keyboards (scancode 0x73)
                if (scancode == 0x73) c = shift_pressed ? '?' : '/';
            }
        }
        
        if (scancode == 0x48) c = 'W';
        if (scancode == 0x50) c = 'S';
        if (scancode == 0x4B) c = 'A';
        if (scancode == 0x4D) c = 'D';

        // Numpad Support (Basic)
        if (scancode == 0x4E) c = '+'; // Numpad +
        if (scancode == 0x4A) c = '-'; // Numpad -
        if (scancode == 0x37) c = '*'; // Numpad *

        if (c) {
            last_key = c;
            key_waiting = 1;
        }
    }
}

void keyboard_init() {
    irq_register_handler(33, keyboard_callback);
}
