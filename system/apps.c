#include "apps.h"
#include "../drivers/keyboard.h"
#include "../drivers/pit.h"
#include "../drivers/rtc.h"
#include "../drivers/vga.h"
#include "../drivers/vga_graphics.h"
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

// --- Maze Quest 3D ---
#define DOOM_MAP_W 16
#define DOOM_MAP_H 16
static const char d_map[] = "################"
                            "#P.............#"
                            "#.##########.#.#"
                            "#.#........#.#.#"
                            "#.#..####..#.#.#"
                            "#.#..#..#..#.#.#"
                            "#....#..#......#"
                            "#.####..####.#.#"
                            "#.####..####.#.#"
                            "#........#.....#"
                            "#.##########.#.#"
                            "#.#........#.#.#"
                            "#.#..####..#.#.#"
                            "#.#.........#.E#"
                            "#..............#"
                            "################";

static uint8_t wall_tex[64 * 64];

static void generate_textures() {
  for (int y = 0; y < 64; y++) {
    for (int x = 0; x < 64; x++) {
      if (x == 0 || y == 0 || y == 31 || (y < 31 && x == 31))
        wall_tex[y * 64 + x] = 20; // Gray
      else
        wall_tex[y * 64 + x] = 40; // Brick
    }
  }
}

void app_maze() {
  shell_active = 0;
  generate_textures();
  vga_set_mode13h();

  uint8_t *backbuffer = (uint8_t *)0x200000;
  uint8_t *screen = (uint8_t *)0xA0000;

  float posX = 1.5f, posY = 1.5f;
  float dirX = 1.0f, dirY = 0.0f;
  float planeX = 0.0f, planeY = 0.66f;

  int show_map = 0;

  RTCTime startTime;
  rtc_get_time(&startTime);
  uint32_t startTotalSec =
      startTime.hour * 3600 + startTime.minute * 60 + startTime.second;

  while (1) {
    if (d_map[(int)posY * DOOM_MAP_W + (int)posX] == 'E') {
      RTCTime endTime;
      rtc_get_time(&endTime);
      uint32_t endTotalSec =
          endTime.hour * 3600 + endTime.minute * 60 + endTime.second;
      uint32_t elapsed = endTotalSec - startTotalSec;

      vga_set_text_mode();
      vga_clear();
      vga_set_color(10, 0);
      vga_puts("\n\n\n\n       *************************************\n");
      vga_puts("       *        VICTORY! ESCAPED!          *\n");
      vga_puts("       *************************************\n\n");
      vga_set_color(15, 0);
      vga_puts("       Time taken: ");
      char buf[16];
      kitoa(elapsed, buf);
      vga_puts(buf);
      vga_puts(" seconds\n\n");
      vga_puts("       Press any key to return to shell...");
      shell_active = 0;
      while (!key_waiting)
        asm volatile("pause");
      key_waiting = 0;
      break;
    }

    for (int i = 0; i < 320 * 100; i++)
      backbuffer[i] = 19;
    for (int i = 320 * 100; i < 320 * 200; i++)
      backbuffer[i] = 24;

    for (int x = 0; x < 320; x++) {
      float cameraX = 2.0f * x / 320.0f - 1.0f;
      float rayDirX = dirX + planeX * cameraX;
      float rayDirY = dirY + planeY * cameraX;
      int mapX = (int)posX, mapY = (int)posY;
      float sideDistX, sideDistY;
      float deltaDistX =
          (rayDirX == 0) ? 1e30
                         : ((rayDirX > 0) ? 1.0f / rayDirX : -1.0f / rayDirX);
      float deltaDistY =
          (rayDirY == 0) ? 1e30
                         : ((rayDirY > 0) ? 1.0f / rayDirY : -1.0f / rayDirY);
      float perpWallDist;
      int stepX, stepY, hit = 0, side;

      if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (posX - mapX) * deltaDistX;
      } else {
        stepX = 1;
        sideDistX = (mapX + 1.0f - posX) * deltaDistX;
      }
      if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (posY - mapY) * deltaDistY;
      } else {
        stepY = 1;
        sideDistY = (mapY + 1.0f - posY) * deltaDistY;
      }

      while (hit == 0) {
        if (sideDistX < sideDistY) {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        } else {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        if (d_map[mapY * DOOM_MAP_W + mapX] == '#' ||
            d_map[mapY * DOOM_MAP_W + mapX] == 'E')
          hit = 1;
      }

      if (side == 0)
        perpWallDist = (sideDistX - deltaDistX);
      else
        perpWallDist = (sideDistY - deltaDistY);

      int lineHeight = (int)(200 / perpWallDist);
      int drawStart = -lineHeight / 2 + 100;
      if (drawStart < 0)
        drawStart = 0;
      int drawEnd = lineHeight / 2 + 100;
      if (drawEnd >= 200)
        drawEnd = 199;

      if (d_map[mapY * DOOM_MAP_W + mapX] == 'E') {
        for (int y = drawStart; y < drawEnd; y++)
          backbuffer[y * 320 + x] = 48; // Green
      } else {
        float wallX;
        if (side == 0)
          wallX = posY + perpWallDist * rayDirY;
        else
          wallX = posX + perpWallDist * rayDirX;
        wallX -= (int)wallX;
        int texX = (int)(wallX * 64.0);
        if (side == 0 && rayDirX > 0)
          texX = 63 - texX;
        if (side == 1 && rayDirY < 0)
          texX = 63 - texX;
        float step = 64.0f / lineHeight;
        float texPos = (drawStart - 100 + lineHeight / 2) * step;
        for (int y = drawStart; y < drawEnd; y++) {
          int texY = (int)texPos & 63;
          texPos += step;
          uint8_t color = wall_tex[texY * 64 + texX];
          if (side == 1)
            color /= 2;
          backbuffer[y * 320 + x] = color;
        }
      }
    }
    for (int i = 0; i < 320 * 200; i++)
      screen[i] = backbuffer[i];

    // Draw Map Overlay
    if (show_map) {
      for (int my = 0; my < 16; my++) {
        for (int mx = 0; mx < 16; mx++) {
          uint8_t color = 0;
          if (d_map[my * 16 + mx] == '#')
            color = 15; // White walls
          else if (d_map[my * 16 + mx] == 'E')
            color = 48; // Green exit

          if ((int)posX == mx && (int)posY == my)
            color = 12; // Red player

          if (color != 0) {
            for (int py_ov = 0; py_ov < 4; py_ov++) {
              for (int px_ov = 0; px_ov < 4; px_ov++) {
                screen[(my * 4 + py_ov + 10) * 320 + (mx * 4 + px_ov + 10)] =
                    color;
              }
            }
          }
        }
      }
    }

    if (key_waiting) {
      char c = last_key;
      key_waiting = 0;
      if (c == 'q' || c == 'Q')
        break;
      if (c == 'f' || c == 'F')
        show_map = !show_map;
      float moveSpeed = 0.2f;
      float cosR = 0.985f, sinR = 0.173f, cosL = 0.985f, sinL = -0.173f;
      if (c == 'w' || c == 'W') {
        if (d_map[(int)posY * DOOM_MAP_W + (int)(posX + dirX * moveSpeed)] !=
            '#')
          posX += dirX * moveSpeed;
        if (d_map[(int)(posY + dirY * moveSpeed) * DOOM_MAP_W + (int)posX] !=
            '#')
          posY += dirY * moveSpeed;
      }
      if (c == 's' || c == 'S') {
        if (d_map[(int)posY * DOOM_MAP_W + (int)(posX - dirX * moveSpeed)] !=
            '#')
          posX -= dirX * moveSpeed;
        if (d_map[(int)(posY - dirY * moveSpeed) * DOOM_MAP_W + (int)posX] !=
            '#')
          posY -= dirY * moveSpeed;
      }
      if (c == 'd' || c == 'D') {
        float oldDirX = dirX;
        dirX = dirX * cosR - dirY * sinR;
        dirY = oldDirX * sinR + dirY * cosR;
        float oldPlaneX = planeX;
        planeX = planeX * cosR - planeY * sinR;
        planeY = oldPlaneX * sinR + planeY * cosR;
      }
      if (c == 'a' || c == 'A') {
        float oldDirX = dirX;
        dirX = dirX * cosL - dirY * sinL;
        dirY = oldDirX * sinL + dirY * cosL;
        float oldPlaneX = planeX;
        planeX = planeX * cosL - planeY * sinL;
        planeY = oldPlaneX * sinL + planeY * cosL;
      }
    }
  }
  vga_set_text_mode();
  vga_init();
  shell_active = 1;
}
