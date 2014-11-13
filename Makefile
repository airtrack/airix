COBJS = start.o kernal_main.o gdt.o idt.o pic.o pit.o
ASMOBJS = kernal.o

all: bootloader kernal a_img

clean:
	@ rm -f *.o a.img bootloader.bin kernal.bin

bootloader: bootloader.s
	@ echo "compiling and linking $< ..."
	@ nasm $< -o bootloader.bin

kernal: $(ASMOBJS) $(COBJS)
	@ echo "linking kernal.bin ..."
	@ ld -m elf_i386 -Ttext-seg=0x100000 $(ASMOBJS) $(COBJS) -s -o kernal.bin

%.o: %.s
	@ echo "compiling $< ..."
	@ nasm -felf $< -o $@

%.o: %.c
	@ echo "compiling $< ..."
	@ gcc -std=c99 -m32 -Wall -c $< -o $@

a_img: bootloader kernal
	@ dd if=/dev/zero of=a.img bs=512 count=2880 > /dev/null 2>&1
	@ dd if=bootloader.bin of=a.img conv=notrunc bs=512 count=1 > /dev/null 2>&1
	@ dd if=kernal.bin of=a.img seek=512 conv=notrunc bs=1 > /dev/null 2>&1
