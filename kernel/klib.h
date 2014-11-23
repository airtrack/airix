#ifndef KLIB_H
#define KLIB_H

#include <kernel/base_types.h>

void get_gdtr(void *gdtr);
void set_gdtr(const void *gdtr);
void set_idtr(const void *idtr);

uint8_t in_byte(uint16_t port);
void out_byte(uint16_t port, uint8_t value);

void close_int();
void start_int();

void isr_entry0();
void isr_entry7();

void clear_screen();
void printk(const char *fmt, ...);

#endif // KLIB_H
