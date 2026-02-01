#include "snake.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/pit.h"
#include <stdint.h>

extern void shell_handle_key(char c); // To intercept keys or just use a flag

static int s_x[100], s_y[100];
static int len = 3;
static int dx=1, dy=0;
static int game_over = 0;

void snake_start() {
    vga_clear();
    len = 3; dx = 1; dy = 0; game_over = 0;
    for(int i=0; i<len; i++) { s_x[i] = 10-i; s_y[i] = 10; }
    
    vga_set_color(10, 0);
    vga_puts("SNAKE START! Use WASD. Press Q to Quit.\n");

    while(!game_over) {
        // Simple delay loop
        for(volatile int i=0; i<5000000; i++);

        // Move
        int next_x = s_x[0] + dx;
        int next_y = s_y[0] + dy;

        if (next_x < 0 || next_x >= 80 || next_y < 0 || next_y >= 25) { game_over = 1; break; }

        for(int i=len-1; i>0; i--) {
            s_x[i] = s_x[i-1];
            s_y[i] = s_y[i-1];
        }
        s_x[0] = next_x;
        s_y[0] = next_y;

        // Draw
        vga_clear();
        vga_set_color(14, 0);
        vga_putchar('@'); // Food (stub loc)
        
        vga_set_color(10, 0);
        for(int i=0; i<len; i++) {
            // Need a vga_put_at or similar? Let's use direct buffer for speed
            uint16_t* b = (uint16_t*)0xB8000;
            b[s_y[i]*80 + s_x[i]] = (uint16_t)0x0A << 8 | '#';
        }
        
        // Input check stub (keyboard_init should be active)
        // In a real kernel we'd use a non-blocking key read
    }
    vga_set_color(12, 0);
    vga_puts("\nGAME OVER! Press any key to return.\n");
}
