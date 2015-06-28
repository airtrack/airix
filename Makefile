.PHONY: all clean dir bootloader kernel a_img

A_IMG = a.img
BIN_DIR = bin

BOOTLOADER_ASM = $(wildcard bootloader/*.s)
BOOTLOADER_BIN = bootloader.bin

KERNEL_C = $(wildcard kernel/*.c) $(wildcard mm/*.c) $(wildcard lib/*.c)
KERNEL_ASM = $(wildcard kernel/*.s) $(wildcard lib/*.s)

KERNEL_COBJS = $(subst .c,.o,$(KERNEL_C))
KERNEL_ASMOBJS = $(subst .s,.o,$(KERNEL_ASM))
KERNEL_BIN = kernel.bin

INCLUDE = -I.
CFLAGS = -std=c99 -m32 -Wall -Wextra -fno-builtin -fno-stack-protector $(INCLUDE)

all: dir bootloader kernel a_img disk

clean:
	@ rm -f kernel/*.o mm/*.o lib/*.o $(BIN_DIR)/* $(A_IMG)

dir:
	@ mkdir -p $(BIN_DIR)

bootloader: $(BOOTLOADER_ASM)
	@ echo "compiling $< ..."
	@ nasm $< -o $(BIN_DIR)/$(BOOTLOADER_BIN)

kernel: $(KERNEL_ASMOBJS) $(KERNEL_COBJS)
	@ echo "linking $(BIN_DIR)/$(KERNEL_BIN) ..."
	@ ld -m elf_i386 -Ttext-seg=0xC0100000 $(KERNEL_ASMOBJS) $(KERNEL_COBJS) -s -o $(BIN_DIR)/$(KERNEL_BIN)

$(KERNEL_ASMOBJS) : %.o : %.s
	@ echo "compiling $< ..."
	@ nasm -felf $< -o $@

$(KERNEL_COBJS) : %.o : %.c
	@ echo "compiling $< ..."
	@ gcc $(CFLAGS) -c $< -o $@

a_img: bootloader kernel
	@ echo "making $(A_IMG) ..."
	@ dd if=/dev/zero of=$(A_IMG) bs=512 count=2880 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(BOOTLOADER_BIN) of=$(A_IMG) conv=notrunc bs=512 count=1 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(KERNEL_BIN) of=$(A_IMG) seek=512 conv=notrunc bs=1 > /dev/null 2>&1

disk:
	@ echo "making disk ..."
	@ dd if=/dev/zero of=disk bs=512 count=20160 > /dev/null 2>&1
