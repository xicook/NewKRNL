#include "paging.h"
#include "../../drivers/serial.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_tables[16][1024] __attribute__((aligned(4096))); // 16 * 4MB = 64MB
uint32_t lfb_table[1024] __attribute__((aligned(4096)));

void paging_init() {
    serial_print("MM: Initializing Paging...\n");
    for(int i = 0; i < 1024; i++) page_directory[i] = 2;

    // Identity 64MB
    for (int t = 0; t < 16; t++) {
        for (int i = 0; i < 1024; i++) {
            first_tables[t][i] = ((t * 4 * 1024 * 1024) + (i * 4096)) | 3;
        }
        page_directory[t] = ((uint32_t)first_tables[t]) | 3;
    }

    // VGA Mapping (0xB8000)
    // VGA Mapping (0xB8000)
    first_tables[0][0xB8] = 0xB8001 | 3;
    // Note: Identity 64MB already covers 0xB8000.

    asm volatile("mov %0, %%cr3" :: "r"(page_directory));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    serial_print("MM: Paging Enabled (64MB Identity + VGA)\n");
}
