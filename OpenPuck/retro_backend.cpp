// ╭────────────────────────────────────────╮
// │  Optional Retro Backend Boundary       │
// │  Prepares console state outside ISR    │
// │  context for the selected bond slot.   │
// ╰────────────────────────────────────────╯

#include "retro_backend.h"

#if OPK_RETRO_BACKEND == OPK_RETRO_SNES

#include "bonds.h"
#include "retro_snes.h"
#include "triton.h"

#include <Arduino.h>

static uint16_t g_retroPending;

void retroBackendInit()
{
	g_retroPending = 0;
}

void retroBackendTask()
{
	bool linked = g_slot[OPK_RETRO_SLOT].used &&
		      g_connReplyMs[OPK_RETRO_SLOT] != 0 &&
		      millis() - g_connReplyMs[OPK_RETRO_SLOT] < 300u;
	g_retroPending = retroSnesPressed(g_in[OPK_RETRO_SLOT].buttons, linked,
					  RETRO_SNES_DEFAULT_MAPPING);
}

bool retroBackendPollingActive()
{
	return false;
}

bool retroBackendOwnsModeChords()
{
	return retroBackendPollingActive();
}

#endif
