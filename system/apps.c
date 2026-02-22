#include "apps.h"
#include "../drivers/keyboard.h"
#include "../drivers/pit.h"
#include "../drivers/rtc.h"
#include "../drivers/vga.h"
#include "lib.h"
#include "setup.h"
#include "shell.h"
#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

// --- Matrix App ---
void app_matrix() {
  shell_active = 0;
  vga_clear();
  vga_set_color(2, 0); // Green

  int drops[80];
  for (int i = 0; i < 80; i++)
    drops[i] = -(i % 25);

  while (1) {
    if (key_waiting) {
      char c = last_key;
      key_waiting = 0;
      if (c == 'q' || c == 'Q')
        break;
    }

    for (int x = 0; x < 80; x++) {
      if (drops[x] >= 0 && drops[x] < 25) {
        uint16_t *fb = (uint16_t *)0xB8000;
        fb[drops[x] * 80 + x] = (uint16_t)0x02 << 8 | (33 + (pit_ticks % 94));
      }
      if (drops[x] >= 25) {
        // Clear trail
        uint16_t *fb = (uint16_t *)0xB8000;
        fb[(drops[x] - 25) * 80 + x] = (uint16_t)0x00 << 8 | ' ';
      }

      drops[x]++;
      if (drops[x] > 50)
        drops[x] = 0;
    }

    for (volatile int d = 0; d < 500000; d++)
      ;
  }

  vga_set_color(15, 0);
  vga_clear();
  shell_active = 1;
}

// --- Calc App ---
void app_calc(char *args) {
  if (!args || !*args) {
    vga_puts("Usage: calc <num1> <op> <num2>\n");
    return;
  }
  // Very simple parser for "1 + 1"
  int n1 = 0, n2 = 0;
  char op = 0;
  char *p = args;

  while (*p == ' ')
    p++;
  // Negative numbers? (Simple support)
  int sign1 = 1;
  if (*p == '-') {
    sign1 = -1;
    p++;
  }
  while (*p >= '0' && *p <= '9') {
    n1 = n1 * 10 + (*p - '0');
    p++;
  }
  n1 *= sign1;

  while (*p == ' ')
    p++;
  op = *p++;

  while (*p == ' ')
    p++;
  int sign2 = 1;
  if (*p == '-') {
    sign2 = -1;
    p++;
  }
  while (*p >= '0' && *p <= '9') {
    n2 = n2 * 10 + (*p - '0');
    p++;
  }
  n2 *= sign2;

  int res = 0;
  if (op == '+')
    res = n1 + n2;
  else if (op == '-')
    res = n1 - n2;
  else if (op == '*' || op == 'x' || op == 'X')
    res = n1 * n2;
  else if (op == '/') {
    if (n2 != 0)
      res = n1 / n2;
    else {
      vga_puts("Error: Division by zero\n");
      return;
    }
  } else {
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
static void kgets_edit(char *buf, int max) {
  int idx = 0;
  while (1) {
    while (!key_waiting)
      ;
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

void app_edit(char *filename) {
  if (!filename || !*filename) {
    vga_puts("Usage: edit <filename>\n");
    return;
  }

  vga_set_color(14, 0); // Yellow
  vga_puts("--- Visual Editor (Beta) ---\n");
  vga_puts("Editing: ");
  vga_puts(filename);
  vga_puts("\n");
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
  vga_puts("Timezone: ");
  vga_puts(global_timezone);
  vga_puts("\n\n");
  vga_puts("Press 'Q' to exit...\n\n");

  // Parse timezone offset (simplified: e.g. "-03" or "UTC")
  int offset = 0;
  if (global_timezone[0] == '-' || global_timezone[0] == '+') {
    offset = (global_timezone[1] - '0') * 10 + (global_timezone[2] - '0');
    if (global_timezone[0] == '-')
      offset = -offset;
  }

  while (1) {
    if (key_waiting) {
      char c = last_key;
      key_waiting = 0;
      if (c == 'q' || c == 'Q')
        break;
    }

    RTCTime t;
    rtc_get_time(&t);

    // Apply offset to UTC time (CMOS usually stores UTC)
    int h = (int)t.hour + offset;
    if (h < 0)
      h += 24;
    if (h >= 24)
      h -= 24;

    vga_set_color(15, 0);
    vga_puts("\rCurrent Time: ");

    char val[16];
    if (h < 10)
      vga_putchar('0');
    kitoa(h, val);
    vga_puts(val);
    vga_putchar(':');

    if (t.minute < 10)
      vga_putchar('0');
    kitoa(t.minute, val);
    vga_puts(val);
    vga_putchar(':');

    if (t.second < 10)
      vga_putchar('0');
    kitoa(t.second, val);
    vga_puts(val);

    vga_puts("    "); // Clear trailing characters

    for (volatile int d = 0; d < 10000000; d++)
      ;
  }

  vga_set_color(15, 0);
  vga_clear();
  shell_active = 1;
}

// --- Doom App (ASCII Raycaster) ---
#define DOOM_MAP_W 16
#define DOOM_MAP_H 16
static const char d_map[] = "################"
                            "#..............#"
                            "#.###..###..###.#"
                            "#.#...........#.#"
                            "#.#.###...###.#.#"
                            "#.#.#.......#.#.#"
                            "#...#.......#...#"
                            "#...#.......#...#"
                            "#.###.......###.#"
                            "#...............#"
                            "#.###..###..###.#"
                            "#.#...........#.#"
                            "#.#..####...#.#.#"
                            "#.#.........#.#.#"
                            "#..............#"
                            "################";

// Fixed-point trig table (0-71 degrees, 5-deg steps, scaled by 256)
static const int s_sin[] = {
    0,    22,   44,   66,   87,   108,  128,  147,  164,  181,  196,  209,
    221,  232,  240,  247,  252,  255,  256,  255,  252,  247,  240,  232,
    221,  209,  196,  181,  164,  147,  128,  108,  87,   66,   44,   22,
    0,    -22,  -44,  -66,  -87,  -108, -128, -147, -164, -181, -196, -209,
    -221, -232, -240, -247, -252, -255, -256, -255, -252, -247, -240, -232,
    -221, -209, -196, -181, -164, -147, -128, -108, -87,  -66,  -44,  -22};
static const int s_cos[] = {
    256,  255,  252,  247,  240,  232,  221,  209,  196,  181,  164,  147,
    128,  108,  87,   66,   44,   22,   0,    -22,  -44,  -66,  -87,  -108,
    -128, -147, -164, -181, -196, -209, -221, -232, -240, -247, -252, -255,
    -256, -255, -252, -247, -240, -232, -221, -209, -196, -181, -164, -147,
    -128, -108, -87,  -66,  -44,  -22,  0,    22,   44,   66,   87,   108,
    128,  147,  164,  181,  196,  209,  221,  232,  240,  247,  252,  255};

void app_doom() {
  shell_active = 0;
  vga_clear();

  int px = 2 * 256, py = 2 * 256; // Player pos x256
  int pa = 0;                     // Angle (0-71)

  while (1) {
    // Simple Render
    uint16_t *fb = (uint16_t *)0xB8000;
    for (int x = 0; x < 80; x++) {
      // Ray angle
      int ra = (pa - 6 + (x * 12 / 80) + 72) % 72;
      int rx = px, ry = py;
      int dist = 0;
      int hit = 0;

      while (!hit && dist < 1000) {
        rx += s_cos[ra] / 4;
        ry += s_sin[ra] / 4;
        dist++;
        if (d_map[(ry / 256) * DOOM_MAP_W + (rx / 256)] == '#')
          hit = 1;
      }

      int ceiling = 12 - (2560 / (dist + 1));
      if (ceiling < 0)
        ceiling = 0;
      int floor = 25 - ceiling;

      for (int y = 0; y < 25; y++) {
        if (y < ceiling)
          fb[y * 80 + x] = 0x0020; // Ceiling
        else if (y > floor)
          fb[y * 80 + x] = 0x082E; // Floor '.'
        else {
          char c = '#';
          if (dist > 300)
            c = 'X';
          if (dist > 600)
            c = '=';
          if (dist > 900)
            c = '.';
          fb[y * 80 + x] = (uint16_t)0x0F00 | c;
        }
      }
    }

    vga_set_color(14, 0);
    vga_puts("\n DOOM (Beta) - WASD to move, Q to exit");

    // Input
    if (key_waiting) {
      char c = last_key;
      key_waiting = 0;
      if (c == 'q' || c == 'Q')
        break;
      if (c == 'w' || c == 'W') {
        int nx = px + s_cos[pa] / 2;
        int ny = py + s_sin[pa] / 2;
        if (d_map[(ny / 256) * DOOM_MAP_W + (nx / 256)] != '#') {
          px = nx;
          py = ny;
        }
      }
      if (c == 's' || c == 'S') {
        int nx = px - s_cos[pa] / 2;
        int ny = py - s_sin[pa] / 2;
        if (d_map[(ny / 256) * DOOM_MAP_W + (nx / 256)] != '#') {
          px = nx;
          py = ny;
        }
      }
      if (c == 'a' || c == 'A')
        pa = (pa - 2 + 72) % 72;
      if (c == 'd' || c == 'D')
        pa = (pa + 2) % 72;
    }
    for (volatile int d = 0; d < 1000000; d++)
      ;
  }

  vga_set_color(15, 0);
  vga_clear();
  shell_active = 1;
}
