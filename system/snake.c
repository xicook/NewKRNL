#include "snake.h"
#include "shell.h"
#include "lib.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include <stdint.h>

static int s_x[2000], s_y[2000];
static int len = 5;
static int dx = 1, dy = 0;
static int food_x = 15, food_y = 15;
static int score = 0;

static uint32_t seed = 123;

static void update_food() {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    food_x = (seed % 78) + 1;
    food_y = ((seed >> 8) % 23) + 1;
}

void snake_start() {
    shell_active = 0;
    vga_clear();
    
    len = 5; 
    dx = 1; 
    dy = 0; 
    score = 0;
    
    for(int i=0; i<len; i++) { 
        s_x[i] = 10-i; 
        s_y[i] = 10; 
    }
    
    update_food();

    int running = 1;

    while(running) {
        // Handle input
        if (key_waiting) {
            char c = last_key;
            key_waiting = 0;
            
            if (c == 'w' || c == 'W') { 
                if(dy != 1) { dx = 0; dy = -1; } 
            }
            else if (c == 's' || c == 'S') { 
                if(dy != -1) { dx = 0; dy = 1; } 
            }
            else if (c == 'a' || c == 'A') { 
                if(dx != 1) { dx = -1; dy = 0; } 
            }
            else if (c == 'd' || c == 'D') { 
                if(dx != -1) { dx = 1; dy = 0; } 
            }
            else if (c == 'q' || c == 'Q') { 
                running = 0; 
                break; 
            }
        }

        // Simple delay (much slower speed)
        for(volatile int d=0; d<15000000; d++);

        // Clear screen (white background)
        uint16_t* fb = (uint16_t*)0xB8000;
        for(int i=0; i<80*25; i++) {
            fb[i] = 0x7020; // White background, black space
        }

        // Game logic
        int next_x = s_x[0] + dx;
        int next_y = s_y[0] + dy;

        // Collision with walls
        if (next_x < 0 || next_x >= 80 || next_y < 0 || next_y >= 25) { 
            running = 0; 
            break; 
        }
        
        // Collision with self
        for(int i=1; i<len; i++) {
            if (next_x == s_x[i] && next_y == s_y[i]) { 
                running = 0; 
                break; 
            }
        }
        if (!running) break;

        // Food collision
        if (next_x == food_x && next_y == food_y) {
            len++;
            score += 10;
            update_food();
        } else {
            // Move snake (only if not eating)
            for(int i=len-1; i>0; i--) {
                s_x[i] = s_x[i-1];
                s_y[i] = s_y[i-1];
            }
        }
        
        s_x[0] = next_x;
        s_y[0] = next_y;

        // Render food (red)
        fb[food_y * 80 + food_x] = 0x7C2A; // Red on white, asterisk

        // Render snake (colorful)
        for(int i=0; i<len; i++) {
            uint8_t color = 10 + (i % 6); // Bright colors
            fb[s_y[i] * 80 + s_x[i]] = ((uint16_t)(0x70 | color) << 8) | '#';
        }
    }

    // Game over
    vga_set_color(15, 0);
    vga_clear();
    vga_set_color(12, 0);
    vga_puts("\n\n GAME OVER!\n\n");
    vga_set_color(15, 0);
    vga_puts(" Score: ");
    char s_buf[16];
    kitoa(score, s_buf);
    vga_puts(s_buf);
    vga_puts("\n\n Press any key to return to shell...\n");
    
    key_waiting = 0;
    while(!key_waiting) { 
        asm volatile("pause"); 
    }
    key_waiting = 0;
    
    vga_set_color(15, 0);
    vga_clear();
    shell_active = 1;
}
