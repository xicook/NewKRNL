#include "vga_graphics.h"
#include <stdint.h>

#define VGA_ADDR_13H 0xA0000
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

static uint8_t saved_font[4096];
static int font_saved = 0;

static void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static uint8_t inb(uint16_t port) {
  uint8_t val;
  asm volatile("inb %1, %0" : "=a"(val) : "dN"(port));
  return val;
}

static void write_regs(uint8_t *regs) {
  // Write MISC
  asm volatile("outb %0, %1" : : "a"(*(regs++)), "dN"(VGA_MISC_WRITE));

  // Write SEQUENCER
  for (uint8_t i = 0; i < 5; i++) {
    asm volatile("outb %0, %1" : : "a"(i), "dN"(VGA_SEQ_INDEX));
    asm volatile("outb %0, %1" : : "a"(*(regs++)), "dN"(VGA_SEQ_DATA));
  }

  // Unprotect CRTC regs
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x03), "dN"(VGA_CRTC_INDEX));
  uint8_t val;
  asm volatile("inb %1, %0" : "=a"(val) : "dN"(VGA_CRTC_DATA));
  asm volatile("outb %0, %1"
               :
               : "a"((uint8_t)(val | 0x80)), "dN"(VGA_CRTC_DATA));
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x11), "dN"(VGA_CRTC_INDEX));
  asm volatile("inb %1, %0" : "=a"(val) : "dN"(VGA_CRTC_DATA));
  asm volatile("outb %0, %1"
               :
               : "a"((uint8_t)(val & ~0x80)), "dN"(VGA_CRTC_DATA));

  regs[0x11] |= 0x80;

  // Write CRTC
  for (uint8_t i = 0; i < 25; i++) {
    asm volatile("outb %0, %1" : : "a"(i), "dN"(VGA_CRTC_INDEX));
    asm volatile("outb %0, %1" : : "a"(*(regs++)), "dN"(VGA_CRTC_DATA));
  }

  // Write GRAPHICS CONTROLLER
  for (uint8_t i = 0; i < 9; i++) {
    asm volatile("outb %0, %1" : : "a"(i), "dN"(VGA_GC_INDEX));
    asm volatile("outb %0, %1" : : "a"(*(regs++)), "dN"(VGA_GC_DATA));
  }

  // Write ATTRIBUTE CONTROLLER
  for (uint8_t i = 0; i < 21; i++) {
    asm volatile("inb %1, %0" : "=a"(val) : "dN"(VGA_INSTAT_READ));
    asm volatile("outb %0, %1" : : "a"(i), "dN"(VGA_AC_INDEX));
    asm volatile("outb %0, %1" : : "a"(*(regs++)), "dN"(VGA_AC_WRITE));
  }

  // Lock palette and enable display
  asm volatile("inb %1, %0" : "=a"(val) : "dN"(VGA_INSTAT_READ));
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x20), "dN"(VGA_AC_INDEX));
}

uint8_t mode_13h_regs[] = {
    /* MISC */ 0x63,
    /* SEQ */ 0x03,
    0x01,
    0x0F,
    0x00,
    0x0E,
    /* CRTC */ 0x5F,
    0x4F,
    0x50,
    0x82,
    0x54,
    0x80,
    0xBF,
    0x1F,
    0x00,
    0x41,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x9C,
    0x0E,
    0x8F,
    0x28,
    0x40,
    0x96,
    0xB9,
    0xA3,
    0xFF,
    /* GC */ 0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x40,
    0x05,
    0x0F,
    0xFF,
    /* AC */ 0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x0C,
    0x0D,
    0x0E,
    0x0F,
    0x41,
    0x00,
    0x0F,
    0x00,
    0x00};

uint8_t mode_text_regs[] = {
    /* MISC */ 0x67,
    /* SEQ */ 0x03,
    0x01,
    0x03,
    0x00,
    0x02,
    /* CRTC */ 0x5F,
    0x4F,
    0x50,
    0x82,
    0x55,
    0x81,
    0xBF,
    0x1F,
    0x00,
    0x4F,
    0x0D,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x00,
    0x9C,
    0x8E,
    0x8F,
    0x28,
    0x1F,
    0x96,
    0xB9,
    0xA3,
    0xFF,
    /* GC */ 0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x10,
    0x0E,
    0x00,
    0xFF,
    /* AC */ 0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x0C,
    0x0D,
    0x0E,
    0x0F,
    0x0C,
    0x00,
    0x0F,
    0x08,
    0x00};

void vga_save_font() {
  // Save state
  outb(VGA_GC_INDEX, 0x04);
  uint8_t old_gc4 = inb(VGA_GC_DATA);
  outb(VGA_GC_INDEX, 0x05);
  uint8_t old_gc5 = inb(VGA_GC_DATA);
  outb(VGA_GC_INDEX, 0x06);
  uint8_t old_gc6 = inb(VGA_GC_DATA);
  outb(VGA_SEQ_INDEX, 0x02);
  uint8_t old_seq2 = inb(VGA_SEQ_DATA);
  outb(VGA_SEQ_INDEX, 0x04);
  uint8_t old_seq4 = inb(VGA_SEQ_DATA);

  // Switch to font read mode
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, 0x07); // Sequential
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, 0x04); // Write only to plane 2
  outb(VGA_GC_INDEX, 0x04);
  outb(VGA_GC_DATA, 0x02); // Read only from plane 2
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, 0x00); // Read mode 0, write mode 0
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, 0x00); // Map to 0xA0000

  uint8_t *vga_mem = (uint8_t *)0xA0000;
  for (int i = 0; i < 4096; i++) {
    saved_font[i] = vga_mem[i];
  }
  font_saved = 1;

  // Restore state
  outb(VGA_GC_INDEX, 0x04);
  outb(VGA_GC_DATA, old_gc4);
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, old_gc5);
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, old_gc6);
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, old_seq2);
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, old_seq4);
}

void vga_restore_font() {
  if (!font_saved)
    return;

  // Switch to font write mode
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, 0x04); // Write only to plane 2
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, 0x07); // Sequential
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, 0x00); // Read mode 0, write mode 0
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, 0x00); // Map to 0xA0000

  uint8_t *vga_mem = (uint8_t *)0xA0000;
  for (int i = 0; i < 4096; i++) {
    vga_mem[i] = saved_font[i];
  }

  // Back to text mode memory layout
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, 0x03); // Enable planes 0 & 1
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, 0x02); // Odd/Even
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, 0x0E); // Map to 0xB8000
}

void vga_set_mode13h() { write_regs(mode_13h_regs); }

void vga_set_text_mode() {
  // Sync wait
  while (inb(VGA_INSTAT_READ) & 0x08)
    ;
  while (!(inb(VGA_INSTAT_READ) & 0x08))
    ;

  // Reset sequencer
  outb(VGA_SEQ_INDEX, 0x00);
  outb(VGA_SEQ_DATA, 0x01);

  write_regs(mode_text_regs);
  vga_restore_font();

  // Restart sequencer
  outb(VGA_SEQ_INDEX, 0x00);
  outb(VGA_SEQ_DATA, 0x03);

  // Reset Attribute Controller
  inb(VGA_INSTAT_READ);
  outb(VGA_AC_INDEX, 0x20);

  // Clear text buffer
  uint16_t *fb = (uint16_t *)0xB8000;
  for (int i = 0; i < 80 * 25; i++) {
    fb[i] = 0x0720;
  }
}

void vga_plot_pixel(int x, int y, uint8_t color) {
  uint8_t *fb = (uint8_t *)VGA_ADDR_13H;
  fb[y * 320 + x] = color;
}

void vga_clear_graphics(uint8_t color) {
  uint8_t *fb = (uint8_t *)VGA_ADDR_13H;
  for (int i = 0; i < 320 * 200; i++)
    fb[i] = color;
}
