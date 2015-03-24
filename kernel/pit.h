#ifndef PIT_H
#define PIT_H

#include <kernel/base_types.h>

/* PIT channels */
enum pit_channel
{
    PIT_CHANNEL_0 = 0x0,
    PIT_CHANNEL_1 = 0x40,
    PIT_CHANNEL_2 = 0x80,
};

/* PIT access mode */
enum pit_access_mode
{
    PIT_ACCESS_LATCH = 0x0,
    PIT_ACCESS_LOBYTE = 0x10,
    PIT_ACCESS_HIBYTE = 0x20,
    PIT_ACCESS_LO_HI = 0x30,
};

/* PIT operating mode */
enum pit_operating_mode
{
    PIT_OPERATING_MODE_0 = 0x0, /* Interrupt on terminal count */
    PIT_OPERATING_MODE_1 = 0x2, /* Hardware re-triggerable one-shot */
    PIT_OPERATING_MODE_2 = 0x4, /* Rate generator */
    PIT_OPERATING_MODE_3 = 0x6, /* Square wave generator */
    PIT_OPERATING_MODE_4 = 0x8, /* Sofeware triggered strobe */
    PIT_OPERATING_MODE_5 = 0xA, /* Hardware triggered strobe */
};

/* PIT BCD/Binary mode */
enum pit_mode
{
    PIT_BINARY_MODE = 0x0,
    PIT_BCD_MODE = 0x1,
};

/* PIT ports */
enum pit_port
{
    PIT_CHANNEL0_PORT = 0x40,
    PIT_CHANNEL1_PORT = 0x41,
    PIT_CHANNEL2_PORT = 0x42,
    PIT_MODE_CMD_PORT = 0x43,
};

#define PIT_BASE_HZ 1193182
#define PIT_RELOAD_MAX 65535

/* PIT init function, set IRQ0 timer frequency */
void init_pit(uint32_t hz);

#endif // PIT_H
