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
    0x00,
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

void vga_set_mode13h() { write_regs(mode_13h_regs); }

void vga_set_text_mode() {
  // Reset sequencer for stable mode switch
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x00), "dN"(VGA_SEQ_INDEX));
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x01), "dN"(VGA_SEQ_DATA));

  write_regs(mode_text_regs);

  // Restart sequencer
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x00), "dN"(VGA_SEQ_INDEX));
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x03), "dN"(VGA_SEQ_DATA));

  // Enable display and reset AC index
  uint8_t val;
  asm volatile("inb %1, %0" : "=a"(val) : "dN"(VGA_INSTAT_READ));
  asm volatile("outb %0, %1" : : "a"((uint8_t)0x20), "dN"(VGA_AC_INDEX));

  // Clear text buffer and set default gray on black
  uint16_t *fb = (uint16_t *)0xB8000;
  for (int i = 0; i < 80 * 25; i++) {
    fb[i] = 0x0720; // Space ' ', Light Gray on Black
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
