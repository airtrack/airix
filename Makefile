all: bootloader kernal a_img

clean:
	@ rm -f *.o a.img bootloader.bin kernal.bin

bootloader: bootloader.s
	nasm bootloader.s -o bootloader.bin

kernal: kernal.o
	ld -m elf_i386 -Ttext-seg=0x100000 kernal.o -s -o kernal.bin

kernal.o: kernal.s
	nasm -felf kernal.s -o kernal.o

a_img: bootloader kernal
	@ dd if=/dev/zero of=a.img bs=512 count=2880 > /dev/null 2>&1
	@ dd if=bootloader.bin of=a.img conv=notrunc bs=512 count=1 > /dev/null 2>&1
	@ dd if=kernal.bin of=a.img seek=512 conv=notrunc bs=1 > /dev/null 2>&1
