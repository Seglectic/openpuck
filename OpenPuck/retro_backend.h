// ╭────────────────────────────────────────╮
// │  Optional Retro Backend Boundary       │
// │  Keeps stock builds as inline no-ops   │
// │  and owns build-time backend checks.   │
// ╰────────────────────────────────────────╯
#pragma once

#define OPK_RETRO_NONE 0
#define OPK_RETRO_SNES 1

#ifndef OPK_RETRO_BACKEND
#define OPK_RETRO_BACKEND OPK_RETRO_NONE
#endif

#ifndef OPK_RETRO_SLOT
#define OPK_RETRO_SLOT 0
#endif

#if OPK_RETRO_BACKEND != OPK_RETRO_NONE && OPK_RETRO_BACKEND != OPK_RETRO_SNES
#error "OPK_RETRO_BACKEND must be OPK_RETRO_NONE or OPK_RETRO_SNES"
#endif

#if OPK_RETRO_SLOT < 0 || OPK_RETRO_SLOT >= 4
#error "OPK_RETRO_SLOT must select bond slot 0..3"
#endif

#if OPK_RETRO_BACKEND == OPK_RETRO_SNES

#if defined(OPK_RETRO_BOARD_NICENANO)
#include "retro_board_nicenano.h"
#endif

#if !defined(OPK_RETRO_LATCH_PIN) || !defined(OPK_RETRO_CLOCK_PIN) || \
	!defined(OPK_RETRO_DATA_PIN)
#error "SNES requires LATCH, CLOCK, and DATA physical nRF pin definitions"
#endif

#if OPK_RETRO_LATCH_PIN < 0 || OPK_RETRO_LATCH_PIN > 47 ||     \
	OPK_RETRO_CLOCK_PIN < 0 || OPK_RETRO_CLOCK_PIN > 47 || \
	OPK_RETRO_DATA_PIN < 0 || OPK_RETRO_DATA_PIN > 47
#error "Retro physical pins must encode P0.00..P1.15 as 0..47"
#endif

#if OPK_RETRO_LATCH_PIN == 0 || OPK_RETRO_LATCH_PIN == 1 ||      \
	OPK_RETRO_LATCH_PIN == 18 || OPK_RETRO_CLOCK_PIN == 0 || \
	OPK_RETRO_CLOCK_PIN == 1 || OPK_RETRO_CLOCK_PIN == 18 || \
	OPK_RETRO_DATA_PIN == 0 || OPK_RETRO_DATA_PIN == 1 ||    \
	OPK_RETRO_DATA_PIN == 18
#error "Retro profile claims an nRF52840 crystal or reset pin"
#endif

#if OPK_RETRO_LATCH_PIN == OPK_RETRO_CLOCK_PIN ||    \
	OPK_RETRO_LATCH_PIN == OPK_RETRO_DATA_PIN || \
	OPK_RETRO_CLOCK_PIN == OPK_RETRO_DATA_PIN
#error "SNES LATCH, CLOCK, and DATA pins must be distinct"
#endif

#if defined(OPK_RETRO_BOARD_PIN_RESERVED)
#if OPK_RETRO_BOARD_PIN_RESERVED(OPK_RETRO_LATCH_PIN) ||     \
	OPK_RETRO_BOARD_PIN_RESERVED(OPK_RETRO_CLOCK_PIN) || \
	OPK_RETRO_BOARD_PIN_RESERVED(OPK_RETRO_DATA_PIN)
#error "SNES profile claims a pin reserved by the selected board"
#endif
#endif

void retroBackendInit();
void retroBackendTask();
bool retroBackendPollingActive();
bool retroBackendOwnsModeChords();

#else

static inline void retroBackendInit()
{
}

static inline void retroBackendTask()
{
}

static inline bool retroBackendPollingActive()
{
	return false;
}

static inline bool retroBackendOwnsModeChords()
{
	return false;
}

#endif
