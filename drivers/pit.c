#include "pit.h"
#include <stdint.h>

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    asm volatile ("outb %1, %0" : : "dN"(0x43), "a"((uint8_t)0x36));
    asm volatile ("outb %1, %0" : : "dN"(0x40), "a"((uint8_t)(divisor & 0xFF)));
    asm volatile ("outb %1, %0" : : "dN"(0x40), "a"((uint8_t)((divisor >> 8) & 0xFF)));
}
