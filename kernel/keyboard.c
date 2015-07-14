#include <kernel/keyboard.h>
#include <kernel/pic.h>
#include <kernel/klib.h>

enum encoder_port
{
    ENCODER_BUF = 0x60,
    ENCODER_CMD_REG = 0x60
};

enum controller_port
{
    CONTROLLER_STATUS = 0x64,
    CONTROLLER_CMD_REG = 0x64
};

enum controller_status
{
    CONTROLLER_OUTPUT_BUF_FULL = 0x1,
    CONTROLLER_INPUT_BUF_FULL = 0x2,
    CONTROLLER_SYSTEM = 0x4,
    CONTROLLER_CMD_DATA = 0x8,
    CONTROLLER_NOT_LOCKED = 0x10,
    CONTROLLER_AUX_BUF = 0x20,
    CONTROLLER_TIMEOUT = 0x40,
    CONTROLLER_PARITY_ERR = 0x80
};

enum encoder_cmd
{
    ENCODER_CMD_SET_LED = 0xED,
};

enum controller_cmd
{
    CONTROLLER_CMD_SELF_TEST = 0xAA,
};

static uint32_t key_states;
static struct key_code_handler key_code_handler;

/* Original XT Scan make code - key code map */
static enum key_code make2key[] =
{
    KEY_UNKNOWN,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LEFT_BRACKET,
    KEY_RIGHT_BRACKET,
    KEY_ENTER,
    KEY_LCTRL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_QUOTE,
    KEY_GRAVE,
    KEY_LSHIFT,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_DOT,
    KEY_SLASH,
    KEY_RSHIFT,
    KEY_KP_ASTERISK,
    KEY_LALT,
    KEY_SPACE,
    KEY_CAPS_LOCK,
    KEY_FN1,
    KEY_FN2,
    KEY_FN3,
    KEY_FN4,
    KEY_FN5,
    KEY_FN6,
    KEY_FN7,
    KEY_FN8,
    KEY_FN9,
    KEY_FN10,
    KEY_NUM_LOCK,
    KEY_SCROLL_LOCK,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,
    KEY_KP_MINUS,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_PLUS,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_0,
    KEY_KP_DOT,
    KEY_UNKNOWN,
    KEY_UNKNOWN,
    KEY_UNKNOWN,
    KEY_FN11,
    KEY_FN12
};

static inline uint8_t encoder_read_buf()
{
    return in_byte(ENCODER_BUF);
}

static inline uint8_t controller_read_status()
{
    return in_byte(CONTROLLER_STATUS);
}

static void wait_encoder_buf_ready()
{
    for (;;)
        if (controller_read_status() & CONTROLLER_OUTPUT_BUF_FULL)
            break;
}

static void wait_controller_ready()
{
    for (;;)
        if (!(controller_read_status() & CONTROLLER_INPUT_BUF_FULL))
            break;
}

static inline void encoder_send_cmd(uint8_t cmd)
{
    /*
     * Wait for keyboard controller to be ready, because commands are sent
     * to keyboard controller first.
     */
    wait_controller_ready();
    out_byte(ENCODER_CMD_REG, cmd);
}

static inline void controller_send_cmd(uint8_t cmd)
{
    wait_controller_ready();
    out_byte(CONTROLLER_CMD_REG, cmd);
}

static void set_leds(bool num, bool caps, bool scroll)
{
    uint8_t data = 0;

    /*
     * Bit 0: scroll lock LED
     * Bit 1: number lock LED
     * Bit 2: caps lock LED
     * 0: off 1: on
     */
    data = scroll ? data | 0x1 : data;
    data = num ? data | 0x2 : data;
    data = caps ? data | 0x4 : data;

    encoder_send_cmd(ENCODER_CMD_SET_LED);
    encoder_send_cmd(data);
}

static bool self_test()
{
    controller_send_cmd(CONTROLLER_CMD_SELF_TEST);
    wait_encoder_buf_ready();
    return encoder_read_buf() == 0x55 ? true : false;
}

static void update_key_states(enum key_code key_code, bool is_break)
{
    uint32_t state;

    switch(key_code)
    {
    case KEY_LSHIFT: state = KEY_STATE_LSHIFT; break;
    case KEY_LCTRL: state = KEY_STATE_LCTRL; break;
    case KEY_LWIN: state = KEY_STATE_LWIN; break;
    case KEY_LALT: state = KEY_STATE_LALT; break;
    case KEY_RSHIFT: state = KEY_STATE_RSHIFT; break;
    case KEY_RCTRL: state = KEY_STATE_RCTRL; break;
    case KEY_RWIN: state = KEY_STATE_RWIN; break;
    case KEY_RALT: state = KEY_STATE_RALT; break;
    case KEY_NUM_LOCK: state = KEY_STATE_NUM_LOCK; break;
    case KEY_CAPS_LOCK: state = KEY_STATE_CAPS_LOCK; break;
    case KEY_SCROLL_LOCK: state = KEY_STATE_SCROLL_LOCK; break;
    default: state = KEY_STATE_NONE; break;
    }

    switch (state)
    {
    case KEY_STATE_NUM_LOCK:
    case KEY_STATE_CAPS_LOCK:
    case KEY_STATE_SCROLL_LOCK:
        if (!is_break)
        {
            bool num, caps, scroll;
            /* Flip state */
            key_states = (key_states & state) ?
                (key_states & ~state) : (key_states | state);

            /* Update LEDs */
            num = (key_states & KEY_STATE_NUM_LOCK) != 0;
            caps = (key_states & KEY_STATE_CAPS_LOCK) != 0;
            scroll = (key_states & KEY_STATE_SCROLL_LOCK) != 0;
            set_leds(num, caps, scroll);
        }
        break;
    case KEY_STATE_NONE:
        break;
    default:
        key_states = is_break ? (key_states & ~state) : (key_states | state);
        break;
    }
}

static bool e0_prefix_scan_to_key_code(uint8_t scan_code,
                                       enum key_code *key_code)
{
    bool is_break = false;
    if (scan_code == 0x47 || scan_code == 0x97)
    {
        *key_code = KEY_HOME;
        is_break = scan_code == 0x97;
    }
    else if (scan_code == 0x2A || scan_code == 0xB7)
    {
        *key_code = KEY_PRINT_SCREEN;
        is_break = scan_code == 0xB7;
        scan_code = encoder_read_buf();
        scan_code = encoder_read_buf();
    }
    else
    {
        if (scan_code > 0x80)
        {
            is_break = true;
            scan_code -= 0x80;
        }

        switch (scan_code)
        {
        case 0x5B: *key_code = KEY_LWIN; break;
        case 0x1D: *key_code = KEY_RCTRL; break;
        case 0x5C: *key_code = KEY_RWIN; break;
        case 0x38: *key_code = KEY_RALT; break;
        case 0x5D: *key_code = KEY_APPS; break;
        case 0x52: *key_code = KEY_INSERT; break;
        case 0x49: *key_code = KEY_PAGE_UP; break;
        case 0x53: *key_code = KEY_DELETE; break;
        case 0x4F: *key_code = KEY_END; break;
        case 0x51: *key_code = KEY_PAGE_DOWN; break;
        case 0x48: *key_code = KEY_U_ARROW; break;
        case 0x4B: *key_code = KEY_L_ARROW; break;
        case 0x50: *key_code = KEY_D_ARROW; break;
        case 0x4D: *key_code = KEY_R_ARROW; break;
        case 0x35: *key_code = KEY_KP_DIVIDE; break;
        case 0x1C: *key_code = KEY_KP_ENTER; break;
        }
    }

    return is_break;
}

static void keyboard_handler()
{
    uint8_t scan_code = 0;
    enum key_code key_code = KEY_UNKNOWN;

    /* Read all scan codes */
    while (controller_read_status() & CONTROLLER_OUTPUT_BUF_FULL)
    {
        bool is_break = false;
        key_code = KEY_UNKNOWN;
        scan_code = encoder_read_buf();

        if (scan_code >= 0x1 && scan_code <= 0x58)
        {
            /* Make code */
            key_code = make2key[scan_code];
        }
        else if (scan_code >= 0x81 && scan_code <= 0xD8)
        {
            /* Break code */
            is_break = true;
            key_code = make2key[scan_code - 0x80];
        }
        else if (scan_code == 0xE0)
        {
            /* E0 prefix scan code */
            scan_code = encoder_read_buf();
            is_break = e0_prefix_scan_to_key_code(scan_code, &key_code);
        }
        else if (scan_code == 0xE1)
        {
            /* E1 prefix scan code is KEY_PAUSE */
            key_code = KEY_PAUSE;
            for (uint32_t i = 0; i < 5; ++i)
                scan_code = encoder_read_buf();
        }

        update_key_states(key_code, is_break);

        /* Ignore key code from keypad when keypad is disabled */
        if (key_code >= KEY_KP_0 && key_code <= KEY_KP_DOT &&
            !kbd_is_key_state_set(key_states, KEY_STATE_NUM_LOCK))
            is_break = true;

        if (!is_break)
            key_code_handler.handler(key_states, key_code,
                                     key_code_handler.data);
    }
}

void kbd_initialize()
{
    printk("[%-8s] self test %s.\n", "Keyboard",
           self_test() ? "succeeded" : "failed");

    key_states = 0;
    set_leds(false, false, false);
    pic_register_isr(IRQ1, keyboard_handler);
}

void kbd_set_key_code_handler(const struct key_code_handler *handler)
{
    key_code_handler = *handler;
}
