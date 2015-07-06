#ifndef CONSOLE_H
#define CONSOLE_H

#include <kernel/keyboard.h>

/* Low byte is ASCII character */
typedef uint16_t console_char_t;
/* Console char consumer prototype */
typedef void (*console_char_consumer_t)(console_char_t, void *);

/* Modify key bits in console_char_t's high byte */
enum console_modify_key
{
    CONSOLE_MODIFY_CTRL = 0x8000,
    CONSOLE_MODIFY_SHIFT = 0x4000,
    CONSOLE_MODIFY_ALT = 0x2000,
};

#define CTRL(c)     ((console_char_t)c | CONSOLE_MODIFY_CTRL)
#define SHIFT(c)    ((console_char_t)c | CONSOLE_MODIFY_SHIFT)
#define ALT(c)      ((console_char_t)c | CONSOLE_MODIFY_ALT)

struct console
{
    console_char_consumer_t char_consumer;
    void *data;
};

/*
 * Convert keyboard's key code to console_char_t, data is
 * a pointer of struct console, the converted console_char_t
 * will be consumed by the struct console
 */
void console_key_code_handler(uint32_t key_states,
                              enum key_code key_code,
                              void *data);

#endif /* CONSOLE_H */
