#include <kernel/base_types.h>
#include <kernel/klib.h>
#include <lib/stdio.h>
#include <lib/string.h>

#define VIDEO_MEMORY (KERNEL_BASE + 0xB8000)
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

void printk(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    print_string(buf);

    update_cursor();
}
