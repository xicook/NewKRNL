CC = gcc
LD = ld
AS = nasm

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -O2 -Wall
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

OBJS = kernel.o \
       system/cpu/gdt.o system/cpu/idt.o system/cpu/interrupt_stubs.o system/cpu/isr.o \
       system/mm/paging.o \
       drivers/vga.o drivers/serial.o drivers/keyboard.o drivers/pit.o drivers/rtc.o \
       system/shell.o system/vfs.o system/snake.o system/setup.o system/lib.o system/apps.o

all: os.img

os.img: boot.bin kernel.bin
	dd if=/dev/zero of=os.img bs=512 count=2880
	dd if=boot.bin of=os.img conv=notrunc
	dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc

boot.bin: boot.asm
	$(AS) -f bin $< -o $@

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

system/cpu/interrupt_stubs.o: system/cpu/interrupt_stubs.asm
	$(AS) -f elf32 $< -o $@

clean:
	rm -f *.bin *.o os.img drivers/*.o system/*.o system/cpu/*.o system/mm/*.o

run: os.img
	qemu-system-i386 -drive format=raw,file=os.img,index=0,if=floppy -m 128M -serial stdio
