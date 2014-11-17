#include <kernal/pit.h>
#include <kernal/klib.h>

void init_pit(uint32_t hz)
{
    /* PIT counter 0 valid value range is [2, 65535] */
    uint32_t reload;
    uint32_t remainder;

    /* Calculate reload value */
    if (hz == 0) hz = 1;
    reload = PIT_BASE_HZ / hz;
    remainder = PIT_BASE_HZ % hz;

    /* If remainder more than half, then round up */
    if (remainder > hz / 2)
        ++reload;

    /* If hz >>> PIT_BASE_HZ, set 2 to reload(1 is invalid value) */
    if (reload <= 1)
        reload = 2;

    /* Set reload as 0(means 65536) when reload > PIT_RELOAD_MAX */
    if (reload > PIT_RELOAD_MAX)
        reload = 0;

    /* Init PIT */
    out_byte(PIT_MODE_CMD_PORT, PIT_CHANNEL_0 | PIT_ACCESS_LO_HI
            | PIT_OPERATING_MODE_2 | PIT_BINARY_MODE);

    /* Set reload value */
    out_byte(PIT_CHANNEL0_PORT, (uint8_t)(reload & 0xFF));
    out_byte(PIT_CHANNEL0_PORT, (uint8_t)((reload >> 8) & 0xFF));
}
