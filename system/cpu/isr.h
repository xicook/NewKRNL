#ifndef ISR_H
#define ISR_H
#include <stdint.h>

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} Registers;

typedef void (*Handler)(Registers*);

void isr_init();
void irq_register_handler(int n, Handler h);
#endif
