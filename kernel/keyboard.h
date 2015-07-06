#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kernel/base.h>

enum key_code
{
    /* Alphabet keys */
    KEY_A = 'a',
    KEY_B = 'b',
    KEY_C = 'c',
    KEY_D = 'd',
    KEY_E = 'e',
    KEY_F = 'f',
    KEY_G = 'g',
    KEY_H = 'h',
    KEY_I = 'i',
    KEY_J = 'j',
    KEY_K = 'k',
    KEY_L = 'l',
    KEY_M = 'm',
    KEY_N = 'n',
    KEY_O = 'o',
    KEY_P = 'p',
    KEY_Q = 'q',
    KEY_R = 'r',
    KEY_S = 's',
    KEY_T = 't',
    KEY_U = 'u',
    KEY_V = 'v',
    KEY_W = 'w',
    KEY_X = 'x',
    KEY_Y = 'y',
    KEY_Z = 'z',

    /* Number keys */
    KEY_0 = '0',
    KEY_1 = '1',
    KEY_2 = '2',
    KEY_3 = '3',
    KEY_4 = '4',
    KEY_5 = '5',
    KEY_6 = '6',
    KEY_7 = '7',
    KEY_8 = '8',
    KEY_9 = '9',

    KEY_GRAVE = '`',
    KEY_MINUS = '-',
    KEY_EQUAL = '=',
    KEY_BACKSLASH = '\\',
    KEY_SPACE = ' ',
    KEY_LEFT_BRACKET = '[',
    KEY_RIGHT_BRACKET = ']',
    KEY_SEMICOLON = ';',
    KEY_QUOTE = '\'',
    KEY_COMMA = ',',
    KEY_DOT = '.',
    KEY_SLASH = '/',

    KEY_BACKSPACE = '\b',
    KEY_ENTER = '\r',
    KEY_TAB = '\t',
    KEY_ESC = 0x1B,

    /* Function keys */
    KEY_FN1 = 0x1001,
    KEY_FN2 = 0x1002,
    KEY_FN3 = 0x1003,
    KEY_FN4 = 0x1004,
    KEY_FN5 = 0x1005,
    KEY_FN6 = 0x1006,
    KEY_FN7 = 0x1007,
    KEY_FN8 = 0x1008,
    KEY_FN9 = 0x1009,
    KEY_FN10 = 0x100A,
    KEY_FN11 = 0x100B,
    KEY_FN12 = 0x100C,

    KEY_CAPS_LOCK = 0x2001,

    /* Modify keys */
    KEY_LSHIFT = 0x2002,
    KEY_LCTRL = 0x2003,
    KEY_LWIN = 0x2004,
    KEY_LALT = 0x2005,
    KEY_RSHIFT = 0x2006,
    KEY_RCTRL = 0x2007,
    KEY_RWIN = 0x2008,
    KEY_RALT = 0x2009,

    KEY_PRINT_SCREEN = 0x200A,
    KEY_SCROLL_LOCK = 0x200B,
    KEY_PAUSE = 0x200C,
    KEY_INSERT = 0x200D,
    KEY_HOME = 0x200E,
    KEY_PAGE_UP = 0x200F,
    KEY_DELETE = 0x2010,
    KEY_END = 0x2011,
    KEY_PAGE_DOWN = 0x2012,
    KEY_NUM_LOCK = 0x2013,
    KEY_APPS = 0x2014,

    /* Arrow keys */
    KEY_U_ARROW = 0x3001,
    KEY_L_ARROW = 0x3002,
    KEY_D_ARROW = 0x3003,
    KEY_R_ARROW = 0x3004,

    /* Keypad keys */
    KEY_KP_0 = 0x4000,
    KEY_KP_1 = 0x4001,
    KEY_KP_2 = 0x4002,
    KEY_KP_3 = 0x4003,
    KEY_KP_4 = 0x4004,
    KEY_KP_5 = 0x4005,
    KEY_KP_6 = 0x4006,
    KEY_KP_7 = 0x4007,
    KEY_KP_8 = 0x4008,
    KEY_KP_9 = 0x4009,
    KEY_KP_DIVIDE = 0x400A,
    KEY_KP_ASTERISK = 0x400B,
    KEY_KP_MINUS = 0x400C,
    KEY_KP_PLUS = 0x400D,
    KEY_KP_ENTER = 0x400E,
    KEY_KP_DOT = 0x400F,

    KEY_UNKNOWN,
};

enum key_state
{
    KEY_STATE_NONE = 0x0,
    KEY_STATE_LSHIFT = 0x1,
    KEY_STATE_LCTRL = 0x2,
    KEY_STATE_LWIN = 0x4,
    KEY_STATE_LALT = 0x8,
    KEY_STATE_RSHIFT = 0x10,
    KEY_STATE_RCTRL = 0x20,
    KEY_STATE_RWIN = 0x40,
    KEY_STATE_RALT = 0x80,
    KEY_STATE_NUM_LOCK = 0x100,
    KEY_STATE_CAPS_LOCK = 0x200,
    KEY_STATE_SCROLL_LOCK = 0x400,
};

/*
 * Key code handler prototype:
 * void handler(uint32_t key_states, enum key_code key, void *data);
 */
typedef void (*key_code_handler_t)(uint32_t, enum key_code, void *);

/* Key code handler struct */
struct key_code_handler
{
    key_code_handler_t handler;
    void *data;
};

void kbd_initialize();

void kbd_set_key_code_handler(const struct key_code_handler *handler);

/* Check key is set helper functions */
static inline bool kbd_is_key_state_set(uint32_t key_states, uint32_t check)
{
    return (key_states & check) != 0;
}

static inline bool kbd_is_shift_set(uint32_t key_states)
{
    return kbd_is_key_state_set(key_states,
                                KEY_STATE_LSHIFT | KEY_STATE_RSHIFT);
}

static inline bool kbd_is_ctrl_set(uint32_t key_states)
{
    return kbd_is_key_state_set(key_states,
                                KEY_STATE_LCTRL | KEY_STATE_RCTRL);
}

static inline bool kbd_is_alt_set(uint32_t key_states)
{
    return kbd_is_key_state_set(key_states,
                                KEY_STATE_LALT | KEY_STATE_RALT);
}

#endif // KEYBOARD_H
