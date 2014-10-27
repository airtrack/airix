all: bootloader kernal a_img

clean:
	@ rm -f a.img bootloader.bin kernal.bin

bootloader: bootloader.s
	nasm bootloader.s -o bootloader.bin

kernal: kernal.s
	nasm kernal.s -o kernal.bin

a_img: bootloader kernal
	@ dd if=/dev/zero of=a.img bs=512 count=2880 >& /dev/null
	@ dd if=bootloader.bin of=a.img conv=notrunc bs=512 count=1 >& /dev/null
	@ dd if=kernal.bin of=a.img oseek=512 conv=notrunc bs=1 >& /dev/null
