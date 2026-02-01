#include "isr.h"
#include "idt.h"
#include "../../drivers/serial.h"

static Handler handlers[256];

extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0(); extern void irq1(); extern void irq2(); extern void irq3();
extern void irq4(); extern void irq5(); extern void irq6(); extern void irq7();
extern void irq8(); extern void irq9(); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

void isr_init() {
    uint32_t isrs[] = {
        (uint32_t)isr0, (uint32_t)isr1, (uint32_t)isr2, (uint32_t)isr3,
        (uint32_t)isr4, (uint32_t)isr5, (uint32_t)isr6, (uint32_t)isr7,
        (uint32_t)isr8, (uint32_t)isr9, (uint32_t)isr10, (uint32_t)isr11,
        (uint32_t)isr12, (uint32_t)isr13, (uint32_t)isr14, (uint32_t)isr15,
        (uint32_t)isr16, (uint32_t)isr17, (uint32_t)isr18, (uint32_t)isr19,
        (uint32_t)isr20, (uint32_t)isr21, (uint32_t)isr22, (uint32_t)isr23,
        (uint32_t)isr24, (uint32_t)isr25, (uint32_t)isr26, (uint32_t)isr27,
        (uint32_t)isr28, (uint32_t)isr29, (uint32_t)isr30, (uint32_t)isr31
    };
    for (int i = 0; i < 32; i++) idt_set_gate(i, isrs[i], 0x08, 0x8E);

    // PIC Remap
    asm volatile ("outb %1, %0" : : "dN"(0x20), "a"((uint8_t)0x11));
    asm volatile ("outb %1, %0" : : "dN"(0xA0), "a"((uint8_t)0x11));
    asm volatile ("outb %1, %0" : : "dN"(0x21), "a"((uint8_t)0x20));
    asm volatile ("outb %1, %0" : : "dN"(0xA1), "a"((uint8_t)0x28));
    asm volatile ("outb %1, %0" : : "dN"(0x21), "a"((uint8_t)0x04));
    asm volatile ("outb %1, %0" : : "dN"(0xA1), "a"((uint8_t)0x02));
    asm volatile ("outb %1, %0" : : "dN"(0x21), "a"((uint8_t)0x01));
    asm volatile ("outb %1, %0" : : "dN"(0xA1), "a"((uint8_t)0x01));
    asm volatile ("outb %1, %0" : : "dN"(0x21), "a"((uint8_t)0x00));
    asm volatile ("outb %1, %0" : : "dN"(0xA1), "a"((uint8_t)0x00));

    uint32_t irqs[] = {
        (uint32_t)irq0, (uint32_t)irq1, (uint32_t)irq2, (uint32_t)irq3,
        (uint32_t)irq4, (uint32_t)irq5, (uint32_t)irq6, (uint32_t)irq7,
        (uint32_t)irq8, (uint32_t)irq9, (uint32_t)irq10, (uint32_t)irq11,
        (uint32_t)irq12, (uint32_t)irq13, (uint32_t)irq14, (uint32_t)irq15
    };
    for (int i = 0; i < 16; i++) idt_set_gate(32 + i, irqs[i], 0x08, 0x8E);
}

void isr_handler(Registers* r) {
    serial_print("CPU: Exception!\n");
    while(1);
}

void irq_handler(Registers* r) {
    if (r->int_no >= 40) asm volatile ("outb %1, %0" : : "dN"(0xA0), "a"((uint8_t)0x20));
    asm volatile ("outb %1, %0" : : "dN"(0x20), "a"((uint8_t)0x20));
    if (handlers[r->int_no]) handlers[r->int_no](r);
}

void irq_register_handler(int n, Handler h) { handlers[n] = h; }
