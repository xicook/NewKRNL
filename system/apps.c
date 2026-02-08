#include "apps.h"
#include "shell.h"
#include "lib.h"
#include "vfs.h"
#include "setup.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/pit.h"
#include "../drivers/rtc.h"
#include <stdint.h>
#include <stddef.h>

// --- Matrix App ---
void app_matrix() {
    shell_active = 0;
    vga_clear();
    vga_set_color(2, 0); // Green

    int drops[80];
    for (int i = 0; i < 80; i++) drops[i] = -(i % 25);

    while (1) {
        if (key_waiting) {
            char c = last_key;
            key_waiting = 0;
            if (c == 'q' || c == 'Q') break;
        }

        for (int x = 0; x < 80; x++) {
            if (drops[x] >= 0 && drops[x] < 25) {
                uint16_t* fb = (uint16_t*)0xB8000;
                fb[drops[x] * 80 + x] = (uint16_t)0x02 << 8 | (33 + (pit_ticks % 94));
            }
            if (drops[x] >= 25) {
                // Clear trail
                uint16_t* fb = (uint16_t*)0xB8000;
                fb[(drops[x]-25) * 80 + x] = (uint16_t)0x00 << 8 | ' ';
            }
            
            drops[x]++;
            if (drops[x] > 50) drops[x] = 0;
        }

        for (volatile int d = 0; d < 500000; d++);
    }

    vga_set_color(15, 0);
    vga_clear();
    shell_active = 1;
}

// --- Calc App ---
void app_calc(char* args) {
    if (!args || !*args) {
        vga_puts("Usage: calc <num1> <op> <num2>\n");
        return;
    }
    // Very simple parser for "1 + 1"
    int n1 = 0, n2 = 0;
    char op = 0;
    char* p = args;
    
    while(*p == ' ') p++;
    // Negative numbers? (Simple support)
    int sign1 = 1;
    if (*p == '-') { sign1 = -1; p++; }
    while(*p >= '0' && *p <= '9') { n1 = n1 * 10 + (*p - '0'); p++; }
    n1 *= sign1;

    while(*p == ' ') p++;
    op = *p++;
    
    while(*p == ' ') p++;
    int sign2 = 1;
    if (*p == '-') { sign2 = -1; p++; }
    while(*p >= '0' && *p <= '9') { n2 = n2 * 10 + (*p - '0'); p++; }
    n2 *= sign2;
    
    int res = 0;
    if (op == '+') res = n1 + n2;
    else if (op == '-') res = n1 - n2;
    else if (op == '*' || op == 'x' || op == 'X') res = n1 * n2;
    else if (op == '/') { 
        if(n2 != 0) res = n1 / n2; 
        else { vga_puts("Error: Division by zero\n"); return; } 
    }
    else { 
        vga_puts("Unknown operator: ");
        vga_putchar(op);
        vga_puts("\n");
        return; 
    }
    
    char buf[16];
    kitoa(res, buf);
    vga_puts("Result: ");
    vga_puts(buf);
    vga_puts("\n");
}

// --- Edit App ---
static void kgets_edit(char* buf, int max) {
    int idx = 0;
    while(1) {
        while(!key_waiting);
        char c = last_key;
        key_waiting = 0;
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

void app_edit(char* filename) {
    if (!filename || !*filename) {
        vga_puts("Usage: edit <filename>\n");
        return;
    }
    
    vga_set_color(14, 0); // Yellow
    vga_puts("--- Visual Editor (Beta) ---\n");
    vga_puts("Editing: "); vga_puts(filename); vga_puts("\n");
    vga_set_color(15, 0);
    vga_puts("Enter content (End with ENTER):\n> ");
    
    char content[512];
    kgets_edit(content, 512);
    
    if (vfs_mkfile(filename, content) == 0) {
        vga_puts("File saved.\n");
    } else {
        vga_puts("Error saving file.\n");
    }
}

void app_time() {
    shell_active = 0;
    vga_clear();
    vga_set_color(14, 0); // Yellow
    vga_puts("--- Real Time Clock ---\n");
    vga_puts("Timezone: "); vga_puts(global_timezone); vga_puts("\n\n");
    vga_puts("Press 'Q' to exit...\n\n");

    // Parse timezone offset (simplified: e.g. "-03" or "UTC")
    int offset = 0;
    if (global_timezone[0] == '-' || global_timezone[0] == '+') {
        offset = (global_timezone[1] - '0') * 10 + (global_timezone[2] - '0');
        if (global_timezone[0] == '-') offset = -offset;
    }

    while (1) {
        if (key_waiting) {
            char c = last_key;
            key_waiting = 0;
            if (c == 'q' || c == 'Q') break;
        }

        RTCTime t;
        rtc_get_time(&t);

        // Apply offset to UTC time (CMOS usually stores UTC)
        int h = (int)t.hour + offset;
        if (h < 0) h += 24;
        if (h >= 24) h -= 24;

        vga_set_color(15, 0);
        vga_puts("\rCurrent Time: ");
        
        char val[16];
        if (h < 10) vga_putchar('0');
        kitoa(h, val); vga_puts(val);
        vga_putchar(':');
        
        if (t.minute < 10) vga_putchar('0');
        kitoa(t.minute, val); vga_puts(val);
        vga_putchar(':');
        
        if (t.second < 10) vga_putchar('0');
        kitoa(t.second, val); vga_puts(val);
        
        vga_puts("    "); // Clear trailing characters

        for (volatile int d = 0; d < 10000000; d++);
    }

    vga_set_color(15, 0);
    vga_clear();
    shell_active = 1;
}
