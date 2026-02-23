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

typedef struct {
  uint8_t misc;
  uint8_t seq[5];
  uint8_t crtc[25];
  uint8_t gc[9];
  uint8_t ac[21];
  uint8_t dac[16 * 3];
  uint8_t font[4096];
} vga_state_t;

static vga_state_t initial_state;
static int state_saved = 0;

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

void vga_save_state() {
  if (state_saved)
    return;

  initial_state.misc = inb(0x3CC);

  for (int i = 0; i < 5; i++) {
    outb(VGA_SEQ_INDEX, i);
    initial_state.seq[i] = inb(VGA_SEQ_DATA);
  }

  // Unprotect CRTC for reading some regs
  outb(VGA_CRTC_INDEX, 0x11);
  uint8_t old_v = inb(VGA_CRTC_DATA);
  outb(VGA_CRTC_DATA, old_v & 0x7F);
  for (int i = 0; i < 25; i++) {
    outb(VGA_CRTC_INDEX, i);
    initial_state.crtc[i] = inb(VGA_CRTC_DATA);
  }
  outb(VGA_CRTC_DATA, old_v);

  for (int i = 0; i < 9; i++) {
    outb(VGA_GC_INDEX, i);
    initial_state.gc[i] = inb(VGA_GC_DATA);
  }

  for (int i = 0; i < 21; i++) {
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, i);
    initial_state.ac[i] = inb(0x3C1);
  }
  inb(VGA_INSTAT_READ);
  outb(VGA_AC_INDEX, 0x20);

  // Save DAC Palette (16 colors)
  outb(0x3C7, 0);
  for (int i = 0; i < 16 * 3; i++) {
    initial_state.dac[i] = inb(0x3C9);
  }

  // Save Font (Plane 2)
  outb(VGA_SEQ_INDEX, 0x02);
  uint8_t s2 = inb(VGA_SEQ_DATA);
  outb(VGA_SEQ_INDEX, 0x04);
  uint8_t s4 = inb(VGA_SEQ_DATA);
  outb(VGA_GC_INDEX, 0x04);
  uint8_t g4 = inb(VGA_GC_DATA);
  outb(VGA_GC_INDEX, 0x05);
  uint8_t g5 = inb(VGA_GC_DATA);
  outb(VGA_GC_INDEX, 0x06);
  uint8_t g6 = inb(VGA_GC_DATA);

  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, 0x07);
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, 0x04);
  outb(VGA_GC_INDEX, 0x04);
  outb(VGA_GC_DATA, 0x02);
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, 0x00);
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, 0x00);

  uint8_t *v = (uint8_t *)0xA0000;
  for (int i = 0; i < 4096; i++)
    initial_state.font[i] = v[i];

  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, s2);
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, s4);
  outb(VGA_GC_INDEX, 0x04);
  outb(VGA_GC_DATA, g4);
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, g5);
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, g6);

  state_saved = 1;
}

void vga_set_text_mode() {
  if (!state_saved)
    return;

  // Restore Palette
  outb(0x3C8, 0);
  for (int i = 0; i < 16 * 3; i++)
    outb(0x3C9, initial_state.dac[i]);

  // Restore Font
  outb(VGA_SEQ_INDEX, 0x02);
  outb(VGA_SEQ_DATA, 0x04);
  outb(VGA_SEQ_INDEX, 0x04);
  outb(VGA_SEQ_DATA, 0x07);
  outb(VGA_GC_INDEX, 0x05);
  outb(VGA_GC_DATA, 0x00);
  outb(VGA_GC_INDEX, 0x06);
  outb(VGA_GC_DATA, 0x00);
  uint8_t *v = (uint8_t *)0xA0000;
  for (int i = 0; i < 4096; i++)
    v[i] = initial_state.font[i];

  // Restore Regs
  outb(VGA_SEQ_INDEX, 0x00);
  outb(VGA_SEQ_DATA, 0x01); // Reset
  outb(VGA_MISC_WRITE, initial_state.misc);
  for (int i = 0; i < 5; i++) {
    outb(VGA_SEQ_INDEX, i);
    outb(VGA_SEQ_DATA, initial_state.seq[i]);
  }

  outb(VGA_CRTC_INDEX, 0x11);
  outb(VGA_CRTC_DATA, initial_state.crtc[0x11] & 0x7F);
  for (int i = 0; i < 25; i++) {
    outb(VGA_CRTC_INDEX, i);
    outb(VGA_CRTC_DATA, initial_state.crtc[i]);
  }

  for (int i = 0; i < 9; i++) {
    outb(VGA_GC_INDEX, i);
    outb(VGA_GC_DATA, initial_state.gc[i]);
  }

  for (int i = 0; i < 21; i++) {
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, i);
    outb(VGA_AC_WRITE, initial_state.ac[i]);
  }

  outb(VGA_SEQ_INDEX, 0x00);
  outb(VGA_SEQ_DATA, 0x03); // Restart
  inb(VGA_INSTAT_READ);
  outb(VGA_AC_INDEX, 0x20);

  // Clear Screen
  uint16_t *fb = (uint16_t *)0xB8000;
  for (int i = 0; i < 80 * 25; i++)
    fb[i] = 0x0720;
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
