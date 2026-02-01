#include "pit.h"
#include "../system/cpu/isr.h"
#include <stdint.h>

volatile uint32_t pit_ticks = 0;

static void pit_callback(Registers* r) {
    (void)r;
    pit_ticks++;
}

void pit_init(uint32_t frequency) {
    irq_register_handler(0, pit_callback);
    uint32_t divisor = 1193182 / frequency;
    asm volatile ("outb %1, %0" : : "dN"(0x43), "a"((uint8_t)0x36));
    asm volatile ("outb %1, %0" : : "dN"(0x40), "a"((uint8_t)(divisor & 0xFF)));
    asm volatile ("outb %1, %0" : : "dN"(0x40), "a"((uint8_t)((divisor >> 8) & 0xFF)));
}
