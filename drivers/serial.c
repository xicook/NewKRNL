#include "serial.h"
#include <stdint.h>
#define PORT 0x3f8

void serial_init() {
    asm volatile ("outb %1, %0" : : "dN"(PORT + 1), "a"((uint8_t)0x00));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 3), "a"((uint8_t)0x80));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 0), "a"((uint8_t)0x03));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 1), "a"((uint8_t)0x00));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 3), "a"((uint8_t)0x03));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 2), "a"((uint8_t)0xC7));
    asm volatile ("outb %1, %0" : : "dN"(PORT + 4), "a"((uint8_t)0x0B));
}

void serial_put(char c) {
    uint8_t status;
    do { asm volatile ("inb %1, %0" : "=a"(status) : "dN"(PORT + 5)); } while (!(status & 0x20));
    asm volatile ("outb %1, %0" : : "dN"(PORT), "a"((uint8_t)c));
}

void serial_print(const char* s) {
    for (int i = 0; s[i]; i++) serial_put(s[i]);
}
