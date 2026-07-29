// ╭────────────────────────────────────────╮
// │  Pure SNES Protocol Model              │
// │  Maps controller buttons and models a  │
// │  latched 16-bit serial response.        │
// ╰────────────────────────────────────────╯
#pragma once

#include <stdint.h>

enum RetroSnesButton : uint8_t {
	RETRO_SNES_B = 0,
	RETRO_SNES_Y,
	RETRO_SNES_SELECT,
	RETRO_SNES_START,
	RETRO_SNES_UP,
	RETRO_SNES_DOWN,
	RETRO_SNES_LEFT,
	RETRO_SNES_RIGHT,
	RETRO_SNES_A,
	RETRO_SNES_X,
	RETRO_SNES_L,
	RETRO_SNES_R,
	RETRO_SNES_BUTTON_COUNT
};

struct RetroSnesMapping {
	uint32_t source[RETRO_SNES_BUTTON_COUNT];
};

extern const RetroSnesMapping RETRO_SNES_DEFAULT_MAPPING;

bool retroSnesMappingValid(const RetroSnesMapping &mapping);
uint16_t retroSnesPressed(uint32_t puckButtons, bool linkUp,
			  const RetroSnesMapping &mapping);
uint16_t retroSnesWireWord(uint16_t pressed);

struct RetroSnesShift {
	uint16_t wire;
	uint8_t bit;
};

void retroSnesShiftInit(RetroSnesShift &shift);
void retroSnesLatch(RetroSnesShift &shift, uint16_t pressed);
bool retroSnesData(const RetroSnesShift &shift);
void retroSnesClock(RetroSnesShift &shift);
