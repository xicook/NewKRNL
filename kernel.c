#include "drivers/serial.h"
#include "drivers/vga.h"
#include "drivers/pit.h"
#include "drivers/keyboard.h"
#include "system/cpu/gdt.h"
#include "system/cpu/idt.h"
#include "system/cpu/isr.h"
#include "system/mm/paging.h"
#include "system/setup.h"
#include "system/shell.h"

void kernel_main() {
    serial_init();
    serial_print("\nNewKRNL VGA Terminal (V5) Loading...\n");
    
    gdt_init();
    idt_init();
    isr_init();
    paging_init();
    
    vga_init();
    pit_init(100);
    keyboard_init();
    
    asm volatile("sti"); // Enable interrupts early for setup
    setup_run();
    
    shell_active = 1;
    shell_init();

    while(1) {
        asm volatile("hlt");
    }
}

void _start() {
    kernel_main();
}
