#ifndef PIT_H
#define PIT_H

#include <kernal/base_types.h>

/* PIT channels */
#define PIT_CHANNEL_0 0x0
#define PIT_CHANNEL_1 0x40
#define PIT_CHANNEL_2 0x80

/* PIT access mode */
#define PIT_ACCESS_LATCH 0x0
#define PIT_ACCESS_LOBYTE 0x10
#define PIT_ACCESS_HIBYTE 0x20
#define PIT_ACCESS_LO_HI 0x30

/* PIT operating mode */
#define PIT_OPERATING_MODE_0 0x0 /* Interrupt on terminal count */
#define PIT_OPERATING_MODE_1 0x2 /* Hardware re-triggerable one-shot */
#define PIT_OPERATING_MODE_2 0x4 /* Rate generator */
#define PIT_OPERATING_MODE_3 0x6 /* Square wave generator */
#define PIT_OPERATING_MODE_4 0x8 /* Sofeware triggered strobe */
#define PIT_OPERATING_MODE_5 0xa /* Hardware triggered strobe */

/* PIT BCD/Binary mode */
#define PIT_BINARY_MODE 0x0
#define PIT_BCD_MODE 0x1

/* PIT ports */
#define PIT_CHANNEL0_PORT 0x40
#define PIT_CHANNEL1_PORT 0x41
#define PIT_CHANNEL2_PORT 0x42
#define PIT_MODE_CMD_PORT 0x43

#define PIT_BASE_HZ 1193182
#define PIT_RELOAD_MAX 65535

/* PIT init function, set IRQ0 timer frequency */
void init_pit(uint32_t hz);

#endif // PIT_H
