#include <kernel/console.h>

static console_char_t apply_modify_keys(console_char_t c, uint32_t key_states)
{
    /* Caps lock is hold */
    if (kbd_is_key_state_set(key_states, KEY_STATE_CAPS_LOCK))
    {
        /* Change c to upper character */
        if (c >= KEY_A && c <= KEY_Z)
            c -= 32;
    }

    /* Shift is hold */
    if (kbd_is_shift_set(key_states))
    {
        switch (c)
        {
        case KEY_0: c = ')'; break;
        case KEY_1: c = '!'; break;
        case KEY_2: c = '@'; break;
        case KEY_3: c = '#'; break;
        case KEY_4: c = '$'; break;
        case KEY_5: c = '%'; break;
        case KEY_6: c = '^'; break;
        case KEY_7: c = '&'; break;
        case KEY_8: c = '*'; break;
        case KEY_9: c = '('; break;
        case KEY_GRAVE: c = '~'; break;
        case KEY_MINUS: c = '_'; break;
        case KEY_EQUAL: c = '+'; break;
        case KEY_BACKSLASH: c = '|'; break;
        case KEY_LEFT_BRACKET: c = '{'; break;
        case KEY_RIGHT_BRACKET: c = '}'; break;
        case KEY_SEMICOLON: c = ':'; break;
        case KEY_QUOTE: c = '"'; break;
        case KEY_COMMA: c = '<'; break;
        case KEY_DOT: c = '>'; break;
        case KEY_SLASH: c = '?'; break;
        default: c = SHIFT(c); break;
        }
    }

    /* Ctrl is hold */
    if (kbd_is_ctrl_set(key_states))
        c = CTRL(c);

    /* Alt is hold */
    if (kbd_is_alt_set(key_states))
        c = ALT(c);

    return c;
}

void console_key_code_handler(uint32_t key_states,
                              enum key_code key_code,
                              void *data)
{
    struct console *console = data;
    console_char_t c = 0;

    if (key_code < 256)
    {
        /* ASCII char */
        c = apply_modify_keys(key_code, key_states);
    }
    else if (key_code >= KEY_KP_0 && key_code <= KEY_KP_9)
    {
        /* Keypad number keys */
        c = (key_code & 0xFF) + '0';
    }
    else
    {
        switch (key_code)
        {
        case KEY_KP_DIVIDE: c = '/'; break;
        case KEY_KP_ASTERISK: c = '*'; break;
        case KEY_KP_MINUS: c = '-'; break;
        case KEY_KP_PLUS: c = '+'; break;
        case KEY_KP_ENTER: c = '\r'; break;
        case KEY_KP_DOT: c = '.'; break;
        case KEY_DELETE: c = 0x7F; break;
        default: break;
        }
    }

    if (c != 0 && console->char_consumer)
        console->char_consumer(c, console->data);
}
