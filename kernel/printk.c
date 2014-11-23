#include <kernel/base_types.h>
#include <kernel/klib.h>
#include <lib/string.h>
#include <stdarg.h>
#include <stdbool.h>

#define VIDEO_MEMORY 0xB8000
#define COLUMN 80
#define LINE 25
#define COLOR_BLACK_WHITE 0x0F

static uint16_t *video_memory = (uint16_t *)VIDEO_MEMORY;
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

static void update_cursor()
{
    uint16_t pos = cursor_x + cursor_y * COLUMN;
    out_byte(0x3D4, 0x0F);
    out_byte(0x3D5, (uint8_t)(pos & 0xFF));
    out_byte(0x3D4, 0x0E);
    out_byte(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void put_char(uint8_t color, uint8_t c)
{
    uint8_t *pos = (uint8_t *)(video_memory + cursor_y * COLUMN + cursor_x);

    switch (c)
    {
        case '\n':
            cursor_x = 0;
            ++cursor_y;
            break;
        case '\r':
            cursor_x = 0;
            break;
        default:
            *pos++ = c;
            *pos = color;
            ++cursor_x;
            if (cursor_x >= COLUMN)
            {
                ++cursor_y;
                cursor_x = 0;
            }
            break;
    }

    /* Scroll up */
    if (cursor_y >= LINE)
    {
        memcpy(video_memory, video_memory + 1 * COLUMN,
                sizeof(*video_memory) * LINE * COLUMN);
        cursor_y = LINE - 1;
    }
}

void clear_screen()
{
    uint8_t *pos = (uint8_t *)video_memory;

    for (int i = 0; i < COLUMN * LINE; ++i)
    {
        *pos++ = ' ';
        *pos++ = COLOR_BLACK_WHITE;
    }

    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

static void print_string(const char *str)
{
    while (*str)
        put_char(COLOR_BLACK_WHITE, *str++);
}

static void reverse(char *buf, int len)
{
    int l = 0, h = len - 1;
    while (l < h)
    {
        char c = buf[l];
        buf[l++] = buf[h];
        buf[h--] = c;
    }
}

static bool utoa_s(unsigned int v, unsigned int base, char *buf, int len)
{
    char data[32];
    int data_len = 0;

    if (base > 16 || base < 2)
        return false;

    /* Convert to string */
    do
    {
        unsigned int r = v % base;
        v /= base;
        if (r < 10)
            data[data_len++] = '0' + r;
        else
            data[data_len++] = 'A' + r - 10;
    } while (v != 0);

    if (len < data_len + 1)
        return false;

    /* Reverse it */
    reverse(data, data_len);

    memcpy(buf, data, data_len);
    buf[data_len] = '\0';
    return true;
}

static bool itoa_s(int i, unsigned int base, char *buf, int len)
{
    unsigned int v;

    if (base == 10 && i < 0)
    {
        v = -i;
        if (utoa_s(v, base, buf + 1, len - 1))
        {
            *buf = '-';
            return true;
        }
        return false;
    }
    else
    {
        v = i;
        return utoa_s(v, base, buf, len);
    }
}

static inline void print_int(int i, unsigned int base)
{
    char str[33];
    if (itoa_s(i, base, str, sizeof(str)))
        print_string(str);
}

static inline void print_uint(unsigned int i, unsigned int base)
{
    char str[33];
    if (utoa_s(i, base, str, sizeof(str)))
        print_string(str);
}

void printk(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    while (*fmt)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
                case 'c':
                    put_char(COLOR_BLACK_WHITE, va_arg(ap, int));
                    ++fmt;
                    break;
                case 's':
                    print_string(va_arg(ap, char *));
                    ++fmt;
                    break;
                case 'd':
                    print_int(va_arg(ap, int), 10);
                    ++fmt;
                    break;
                case 'x':
                    print_int(va_arg(ap, int), 16);
                    ++fmt;
                    break;
                case 'u':
                    print_uint(va_arg(ap, unsigned int), 10);
                    ++fmt;
                    break;
                case '%':
                    put_char(COLOR_BLACK_WHITE, *fmt++);
                    break;
            }
        }
        else
        {
            put_char(COLOR_BLACK_WHITE, *fmt++);
        }
    }

    va_end(ap);

    update_cursor();
}
