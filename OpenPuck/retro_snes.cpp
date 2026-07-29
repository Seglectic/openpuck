// ╭────────────────────────────────────────╮
// │  Pure SNES Protocol Model              │
// │  Implements fail-closed mapping and    │
// │  deterministic serial-frame state.     │
// ╰────────────────────────────────────────╯

#include "retro_snes.h"
#include "triton.h"

const RetroSnesMapping RETRO_SNES_DEFAULT_MAPPING = {
	{
		TB_B,
		TB_Y,
		TB_VIEW,
		TB_MENU,
		TB_DUP,
		TB_DDN,
		TB_DLF,
		TB_DRT,
		TB_A,
		TB_X,
		TB_LB,
		TB_RB,
	},
};

static const uint32_t RETRO_SNES_ALLOWED_SOURCES =
	TB_A | TB_B | TB_X | TB_Y | TB_VIEW | TB_MENU | TB_DUP | TB_DDN |
	TB_DLF | TB_DRT | TB_LB | TB_RB;

bool retroSnesMappingValid(const RetroSnesMapping &mapping)
{
	uint32_t used = 0;

	for (uint8_t i = 0; i < RETRO_SNES_BUTTON_COUNT; i++) {
		uint32_t source = mapping.source[i];

		if (!source)
			continue;
		if ((source & (source - 1)) ||
		    (source & ~RETRO_SNES_ALLOWED_SOURCES) || (source & used))
			return false;
		used |= source;
	}
	return true;
}

uint16_t retroSnesPressed(uint32_t puckButtons, bool linkUp,
			  const RetroSnesMapping &mapping)
{
	if (!linkUp || !retroSnesMappingValid(mapping))
		return 0;

	uint16_t pressed = 0;
	for (uint8_t i = 0; i < RETRO_SNES_BUTTON_COUNT; i++)
		if (mapping.source[i] && (puckButtons & mapping.source[i]))
			pressed |= (uint16_t)(1u << i);

	uint16_t vertical =
		(uint16_t)((1u << RETRO_SNES_UP) | (1u << RETRO_SNES_DOWN));
	if ((pressed & vertical) == vertical)
		pressed &= (uint16_t)~vertical;

	uint16_t horizontal =
		(uint16_t)((1u << RETRO_SNES_LEFT) | (1u << RETRO_SNES_RIGHT));
	if ((pressed & horizontal) == horizontal)
		pressed &= (uint16_t)~horizontal;

	return pressed;
}

uint16_t retroSnesWireWord(uint16_t pressed)
{
	return (uint16_t)(~pressed | 0xF000u);
}

void retroSnesShiftInit(RetroSnesShift &shift)
{
	shift.wire = 0xFFFFu;
	shift.bit = 16;
}

void retroSnesLatch(RetroSnesShift &shift, uint16_t pressed)
{
	shift.wire = retroSnesWireWord(pressed);
	shift.bit = 0;
}

bool retroSnesData(const RetroSnesShift &shift)
{
	if (shift.bit >= 16)
		return true;
	return (shift.wire & (uint16_t)(1u << shift.bit)) != 0;
}

void retroSnesClock(RetroSnesShift &shift)
{
	if (shift.bit < 16)
		shift.bit++;
}
