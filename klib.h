#ifndef KLIB_H
#define KLIB_H

#include "base_types.h"

void get_gdtr(void *gdtr);
void set_gdtr(const void *gdtr);
void set_idtr(const void *idtr);
uint8_t in_byte(uint16_t port);
void out_byte(uint16_t port, uint8_t value);
void display_char(int line, int column, char c);

#endif // KLIB_H
