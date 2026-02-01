[org 0x7c00]
[bits 16]

; Zero-BIOS VGA Terminal Bootloader (V5)
; Mode: 80x25 Text (Standard)

KERNEL_SEG    equ 0x1000
KERNEL_TARGET equ 0x100000

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Fast A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Load Kernel (BIOS int 0x13)
    mov ax, KERNEL_SEG
    mov es, ax
    mov bx, 0
    mov ah, 0x02
    mov al, 127 ; Load 64KB approx
    mov ch, 0
    mov dh, 0
    mov cl, 2
    int 0x13
    jc disk_err

    ; Pass to PM
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_init

disk_err:
    mov si, msg_disk
    call print
    jmp $

print:
    mov ah, 0x0e
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: ret

[bits 32]
pm_init:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000

    ; Relocate to 1MB
    mov esi, 0x10000
    mov edi, KERNEL_TARGET
    mov ecx, (127 * 512) / 4
    rep movsd

    jmp KERNEL_TARGET

msg_disk db "Disk Error", 0

gdt_null: dq 0
gdt_code: dq 0x00cf9a000000ffff
gdt_data: dq 0x00cf92000000ffff
gdt_end:
gdt_desc:
    dw gdt_end - gdt_null - 1
    dd gdt_null

times 510-($-$$) db 0
dw 0xaa55
