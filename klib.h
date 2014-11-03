#ifndef KLIB_H
#define KLIB_H

void get_gdtr(void *);
void set_gdtr(const void *);
void set_idtr(const void *);
void display_char(int, int, char);

#endif // KLIB_H
