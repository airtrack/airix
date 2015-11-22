.PHONY: all clean dir bootloader kernel libc user a_img

.PRECIOUS: %.o

A_IMG = a.img
DISK = disk
BIN_DIR = bin
LIB_DIR = lib

BOOTLOADER = bootloader.bin
BOOTLOADER_ASM = $(wildcard bootloader/*.s)

KERNEL_C = $(wildcard kernel/*.c) $(wildcard mm/*.c) \
			$(wildcard fs/*.c) $(filter-out lib/crt.c, $(wildcard lib/*.c))
KERNEL_ASM = $(wildcard kernel/*.s) $(filter-out lib/syscall.s, $(wildcard lib/*.s))

KERNEL = kernel.bin
KERNEL_OBJS = $(subst .c,.o,$(KERNEL_C)) $(subst .s,.o,$(KERNEL_ASM))

LIBC = libc.a
LIBC_OBJS = $(subst .c,.o,$(wildcard lib/*.c)) $(subst .s,.o,$(wildcard lib/*.s))

USER_BIN = $(subst user,bin,$(subst .c,,$(wildcard user/*.c)))

INCLUDE = -I. -Ilib
CFLAGS = -std=c99 -m32 -Wall -Wextra -nostdinc -fno-builtin -fno-stack-protector $(INCLUDE)
LFLAGS = -nostdlib -Llib -lc

all: dir bootloader kernel libc user a_img disk

user: libc $(USER_BIN)

clean:
	@ rm -f kernel/*.o mm/*.o fs/*.o lib/*.o lib/*.a user/*.o $(BIN_DIR)/* $(A_IMG) $(DISK)

dir:
	@ mkdir -p $(BIN_DIR)

bootloader: $(BOOTLOADER_ASM)
	@ echo "compiling $< ..."
	@ nasm $< -o $(BIN_DIR)/$(BOOTLOADER)

kernel: $(KERNEL_OBJS)
	@ echo "linking $(BIN_DIR)/$(KERNEL) ..."
	@ ld -m elf_i386 -Ttext-seg=0xC0100000 $(KERNEL_OBJS) -s -o $(BIN_DIR)/$(KERNEL)

libc: $(LIBC_OBJS)
	@ echo "ar $(LIB_DIR)/$(LIBC) ..."
	@ ar -r $(LIB_DIR)/$(LIBC) $(LIBC_OBJS) > /dev/null 2>&1

bin/%: user/%.o
	@ echo "linking $@ ..."
	@ gcc $(CFLAGS) $< $(LFLAGS) -o $@

%.o : %.s
	@ echo "compiling $< ..."
	@ nasm -felf $< -o $@

%.o : %.c
	@ echo "compiling $< ..."
	@ gcc $(CFLAGS) -c $< -o $@

a_img: bootloader kernel
	@ echo "making $(A_IMG) ..."
	@ dd if=/dev/zero of=$(A_IMG) bs=512 count=2880 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(BOOTLOADER) of=$(A_IMG) conv=notrunc bs=512 count=1 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/$(KERNEL) of=$(A_IMG) seek=512 conv=notrunc bs=1 > /dev/null 2>&1

disk:
	@ echo "making $(DISK) ..."
	@ dd if=/dev/zero of=$(DISK) bs=512 count=20160 > /dev/null 2>&1
	@ dd if=$(BIN_DIR)/init of=$(DISK) conv=notrunc bs=1 > /dev/null 2>&1
