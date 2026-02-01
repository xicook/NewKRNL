#include "shell.h"
#include "vfs.h"
#include "snake.h"
#include "setup.h"
#include "lib.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include <stddef.h>

static char buffer[1024];
static int b_idx = 0;
int shell_active = 0;

// No local helpers needed

void shell_init() {
    vfs_init();
    vga_set_color(15, 0);
    vga_puts("\n");
    vga_puts(global_username);
    vga_puts("@NewKRNL:# ");
    b_idx = 0;
}

void shell_update() {
    if (shell_active && key_waiting) {
        char c = last_key;
        key_waiting = 0;
        shell_handle_key(c);
    }
}

static void neofetch() {
    vga_set_color(11, 0);
    vga_puts("   _   _                    \n");
    vga_puts("  | \\ | |                   \n");
    vga_puts("  |  \\| | _____      __     \n");
    vga_puts("  | . ` |/ _ \\ \\ /\\ / /     \n");
    vga_puts("  | |\\  |  __/\\ V  V /      \n");
    vga_puts("  |_| \\_|\\___| \\_/\\_/       \n");
    vga_set_color(15, 0);
    vga_puts("  ---------------------\n");
    vga_puts("  OS: NewKRNL V5 (Zero-BIOS)\n");
    vga_puts("  Kernel: Workstation Core\n");
    vga_puts("  User: "); vga_puts(global_username); vga_puts("\n");
    vga_puts("  Timezone: "); vga_puts(global_timezone); vga_puts("\n");
    vga_puts("  Term: VGA 80x25\n\n");
}

static void execute_command() {
    vga_puts("\n");
    if (b_idx == 0) goto prompt;

    if (kstrcmp(buffer, "help") == 0) {
        vga_puts("Files: ls, mkdir <name>, mkfile <name> [content], rm <name>, cat <name>, cd <name>\n");
        vga_puts("System: sysinfo, whoami, reboot, shutdown, resolution\n");
        vga_puts("Fun: snake, clear\n");
    } else if (kstrcmp(buffer, "clear") == 0) {
        vga_clear();
        goto prompt_no_nl;
    } else if (kstrcmp(buffer, "ls") == 0) {
        vfs_ls();
    } else if (kstrcmp(buffer, "whoami") == 0) {
        vga_puts(global_username); vga_puts("\n");
    } else if (kstrcmp(buffer, "sysinfo") == 0) {
        neofetch();
    } else if (kstrcmp(buffer, "snake") == 0) {
        snake_start();
        vga_clear();
    } else if (kstrcmp(buffer, "reboot") == 0) {
        asm volatile ("outb %1, %0" : : "dN"((uint16_t)0x64), "a"((uint8_t)0xFE));
    } else if (kstrcmp(buffer, "shutdown") == 0) {
        vga_puts("Powering off...\n");
        asm volatile ("outw %1, %0" : : "dN"((uint16_t)0x604), "a"((uint16_t)0x2000)); // QEMU
        asm volatile ("outw %1, %0" : : "dN"((uint16_t)0xB004), "a"((uint16_t)0x2000)); // Bochs/Older QEMU
        while(1) asm volatile("hlt");
    } else if (kstrcmp(buffer, "resolution") == 0) {
        vga_puts("Current resolution: 80x25 (Standard VGA Text Mode)\n");
    } else if (buffer[0] == 'm' && buffer[1] == 'k' && buffer[2] == 'd' && buffer[3] == 'i' && buffer[4] == 'r' && buffer[5] == ' ') {
        vfs_mkdir(buffer + 6);
    } else if (buffer[0] == 'm' && buffer[1] == 'k' && buffer[2] == 'f' && buffer[3] == 'i' && buffer[4] == 'l' && buffer[5] == 'e' && buffer[6] == ' ') {
        char* filename = buffer + 7;
        char* content = NULL;
        for (int i = 7; buffer[i]; i++) {
            if (buffer[i] == ' ') {
                buffer[i] = '\0';
                content = buffer + i + 1;
                break;
            }
        }
        if (vfs_mkfile(filename, content ? content : "") != 0) {
            vga_puts("Error creating file.\n");
        }
    } else if (buffer[0] == 'c' && buffer[1] == 'a' && buffer[2] == 't' && buffer[3] == ' ') {
        char* c = vfs_read(buffer + 4);
        if (c) { vga_puts(c); vga_puts("\n"); }
        else vga_puts("File not found.\n");
    } else if (buffer[0] == 'c' && buffer[1] == 'd' && buffer[2] == ' ') {
        if (vfs_cd(buffer + 3) != 0) vga_puts("Directory not found.\n");
    } else if (buffer[0] == 'r' && buffer[1] == 'm' && buffer[2] == ' ') {
        if (vfs_rm(buffer + 3) != 0) vga_puts("Entry not found.\n");
    } else {
        vga_set_color(12, 0);
        vga_puts("Unknown command: ");
        vga_puts(buffer);
        vga_set_color(15, 0);
        vga_puts("\n");
    }

prompt:
prompt_no_nl:
    b_idx = 0;
    for(int i=0; i<1024; i++) buffer[i] = 0;
    vga_puts(global_username);
    vga_puts("@NewKRNL:# ");
}

void shell_handle_key(char c) {
    if (!shell_active) return;
    if (c == '\b') {
        if (b_idx > 0) {
            b_idx--;
            buffer[b_idx] = 0;
            vga_putchar('\b');
        }
    } else if (c == '\n') {
        execute_command();
    } else if (b_idx < 1000) {
        buffer[b_idx++] = c;
        vga_putchar(c);
    }
}
