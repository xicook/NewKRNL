#include "setup.h"
#include "lib.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include <stddef.h>

char global_username[32] = "root";
char global_timezone[32] = "UTC";

// Simplified blocking char read
static char wait_key() {
    while(!key_waiting);
    key_waiting = 0;
    return last_key;
}

static void kgets(char* buf, int max) {
    int idx = 0;
    while(1) {
        char c = wait_key();
        if (c == '\n') {
            buf[idx] = 0;
            vga_putchar('\n');
            break;
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                vga_putchar('\b');
            }
        } else if (idx < max - 1) {
            buf[idx++] = c;
            vga_putchar(c);
        }
    }
}

void setup_run() {
    vga_clear();
    vga_set_color(11, 0); // Cyan
    vga_puts("=== NewKRNL Setup ===\n\n");
    
    vga_set_color(15, 0);
    vga_puts("Welcome to your Zero-BIOS Workstation.\n");
    vga_puts("Please configure your environment.\n\n");
    
    vga_puts("Desired Username: ");
    kgets(global_username, 32);
    if (global_username[0] == 0) kstrcpy(global_username, "root");
    
    vga_puts("Timezone (e.g. UTC, -03:00): ");
    kgets(global_timezone, 32);
    if (global_timezone[0] == 0) kstrcpy(global_timezone, "UTC");
    
    vga_set_color(10, 0); // Green
    vga_puts("\nSetup Complete! Entering Workspace...\n");
    for(volatile int i=0; i<50000000; i++); // Brief pause
}
