.PHONY: all clean dir bootloader kernal a_img

A_IMG = a.img
BIN_DIR = bin

BOOTLOADER_ASM = $(wildcard bootloader/*.s)
BOOTLOADER_BIN = bootloader.bin

KERNAL_C = $(wildcard kernal/*.c) $(wildcard mm/*.c)
KERNAL_ASM = $(wildcard kernal/*.s)

KERNAL_COBJS = $(subst .c,.o,$(KERNAL_C))
KERNAL_ASMOBJS = $(subst .s,.o,$(KERNAL_ASM))
KERNAL_BIN = kernal.bin

INCLUDE = -I.
CFLAGS = -std=c99 -m32 -Wall $(INCLUDE)

all: dir bootloader kernal a_img

clean:
	@ rm -f kernal/*.o mm/*.o $(BIN_DIR)/* $(A_IMG)

dir:
	@ mkdir -p $(BIN_DIR)

bootloader: $(BOOTLOADER_ASM)
	@ echo "compiling $< ..."
	@ nasm $< -o $(BIN_DIR)/$(BOOTLOADER_BIN)

kernal: $(KERNAL_ASMOBJS) $(KERNAL_COBJS)
	@ echo "linking $(BIN_DIR)/$(KERNAL_BIN) ..."
	@ ld -m elf_i386 -Ttext-seg=0x100000 $(KERNAL_ASMOBJS) $(KERNAL_COBJS) -s -o $(BIN_DIR)/$(KERNAL_BIN)

$(KERNAL_ASMOBJS) : %.o : %.s
	@ echo "compiling $< ..."
	@ nasm -felf $< -o $@

$(KERNAL_COBJS) : %.o : %.c
	@ echo "compiling $< ..."
	@ gcc $(CFLAGS) -c $< -o $@

a_img: bootloader kernal
	@ echo "making $(A_IMG) ..."
	@ dd if=/dev/zero of=$(A_IMG) bs=512 count=2880 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(BOOTLOADER_BIN) of=$(A_IMG) conv=notrunc bs=512 count=1 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(KERNAL_BIN) of=$(A_IMG) seek=512 conv=notrunc bs=1 > /dev/null 2>&1
