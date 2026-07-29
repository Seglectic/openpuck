// ╭────────────────────────────────────────╮
// │  nice!nano Retro Signal Profile        │
// │  Names physical nRF pins shared by the │
// │  documented v1 and v2 header layouts.  │
// ╰────────────────────────────────────────╯
#pragma once

#ifndef OPK_RETRO_LATCH_PIN
#define OPK_RETRO_LATCH_PIN 17
#endif

#ifndef OPK_RETRO_CLOCK_PIN
#define OPK_RETRO_CLOCK_PIN 20
#endif

#ifndef OPK_RETRO_DATA_PIN
#define OPK_RETRO_DATA_PIN 22
#endif

// P0.00/P0.01 are the LF crystal, P0.04 senses the battery, P0.13 controls
// external VCC, and P0.18 is reset. A retro profile must not claim them.
#define OPK_RETRO_BOARD_PIN_RESERVED(PIN) \
	((PIN) == 0 || (PIN) == 1 || (PIN) == 4 || (PIN) == 13 || (PIN) == 18)
