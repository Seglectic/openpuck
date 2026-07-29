// ╭────────────────────────────────────────╮
// │  Native SNES Model Tests               │
// │  Verifies mapping, active-low frames,  │
// │  and latch/clock safety semantics.      │
// ╰────────────────────────────────────────╯

#include "retro_snes.h"
#include "triton.h"

#include <stdint.h>
#include <stdio.h>

static int g_failures;

#define EXPECT_EQ(ACTUAL, EXPECTED)                                            \
	do {                                                                   \
		unsigned long long observed_ = (unsigned long long)(ACTUAL);   \
		unsigned long long expected_ = (unsigned long long)(EXPECTED); \
		if (observed_ != expected_) {                                  \
			fprintf(stderr,                                        \
				"%s:%d: got 0x%llX, expected 0x%llX\n",        \
				__FILE__, __LINE__, observed_, expected_);     \
			g_failures++;                                          \
		}                                                              \
	} while (0)

#define EXPECT_TRUE(VALUE) EXPECT_EQ((VALUE) ? 1 : 0, 1)
#define EXPECT_FALSE(VALUE) EXPECT_EQ((VALUE) ? 1 : 0, 0)

static void testMapping()
{
	EXPECT_TRUE(retroSnesMappingValid(RETRO_SNES_DEFAULT_MAPPING));
	EXPECT_EQ(retroSnesPressed(0, true, RETRO_SNES_DEFAULT_MAPPING), 0);
	EXPECT_EQ(retroSnesWireWord(0), 0xFFFFu);

	for (uint8_t i = 0; i < RETRO_SNES_BUTTON_COUNT; i++) {
		uint32_t source = RETRO_SNES_DEFAULT_MAPPING.source[i];
		uint16_t expected = (uint16_t)(1u << i);

		EXPECT_EQ(retroSnesPressed(source, true,
					   RETRO_SNES_DEFAULT_MAPPING),
			  expected);
		EXPECT_EQ(retroSnesWireWord(expected), (uint16_t)~expected);
	}

	uint32_t combination = TB_B | TB_MENU | TB_DUP | TB_DRT | TB_X;
	uint16_t expected =
		(uint16_t)((1u << RETRO_SNES_B) | (1u << RETRO_SNES_START) |
			   (1u << RETRO_SNES_UP) | (1u << RETRO_SNES_RIGHT) |
			   (1u << RETRO_SNES_X));
	EXPECT_EQ(retroSnesPressed(combination, true,
				   RETRO_SNES_DEFAULT_MAPPING),
		  expected);
	EXPECT_EQ(retroSnesWireWord(expected), (uint16_t)~expected);
	EXPECT_EQ(retroSnesWireWord(expected) & 0xF000u, 0xF000u);
}

static void testMappingSafety()
{
	EXPECT_EQ(retroSnesPressed(TB_A, false, RETRO_SNES_DEFAULT_MAPPING), 0);
	RetroSnesShift shift;
	retroSnesLatch(shift, retroSnesPressed(TB_B, true,
					       RETRO_SNES_DEFAULT_MAPPING));
	EXPECT_FALSE(retroSnesData(shift));
	retroSnesLatch(shift, retroSnesPressed(TB_B, false,
					       RETRO_SNES_DEFAULT_MAPPING));
	for (uint8_t bit = 0; bit < 16; bit++) {
		EXPECT_TRUE(retroSnesData(shift));
		retroSnesClock(shift);
	}

	uint32_t opposing = TB_DUP | TB_DDN | TB_DLF | TB_DRT | TB_A;
	EXPECT_EQ(retroSnesPressed(opposing, true, RETRO_SNES_DEFAULT_MAPPING),
		  (uint16_t)(1u << RETRO_SNES_A));

	RetroSnesMapping invalid = RETRO_SNES_DEFAULT_MAPPING;
	invalid.source[RETRO_SNES_B] = TB_A | TB_B;
	EXPECT_FALSE(retroSnesMappingValid(invalid));
	EXPECT_EQ(retroSnesPressed(TB_A | TB_B, true, invalid), 0);

	invalid = RETRO_SNES_DEFAULT_MAPPING;
	invalid.source[RETRO_SNES_B] = TB_STEAM;
	EXPECT_FALSE(retroSnesMappingValid(invalid));
	EXPECT_EQ(retroSnesPressed(TB_STEAM, true, invalid), 0);

	invalid = RETRO_SNES_DEFAULT_MAPPING;
	invalid.source[RETRO_SNES_B] = TB_A;
	EXPECT_FALSE(retroSnesMappingValid(invalid));
	EXPECT_EQ(retroSnesPressed(TB_A, true, invalid), 0);

	RetroSnesMapping unmapped = RETRO_SNES_DEFAULT_MAPPING;
	unmapped.source[RETRO_SNES_B] = 0;
	EXPECT_TRUE(retroSnesMappingValid(unmapped));
	EXPECT_EQ(retroSnesPressed(TB_B, true, unmapped), 0);
}

static void testShiftOrderAndCompletion()
{
	RetroSnesShift shift;
	retroSnesShiftInit(shift);
	EXPECT_TRUE(retroSnesData(shift));

	uint16_t pressed = 0x0A55u;
	uint16_t wire = retroSnesWireWord(pressed);
	retroSnesLatch(shift, pressed);
	for (uint8_t bit = 0; bit < 16; bit++) {
		EXPECT_EQ(retroSnesData(shift), (wire >> bit) & 1u);
		retroSnesClock(shift);
	}
	EXPECT_TRUE(retroSnesData(shift));
	retroSnesClock(shift);
	retroSnesClock(shift);
	EXPECT_TRUE(retroSnesData(shift));
}

static void testLatchSnapshotAndRestart()
{
	RetroSnesShift shift;
	retroSnesShiftInit(shift);

	uint16_t first = (uint16_t)(1u << RETRO_SNES_B);
	retroSnesLatch(shift, first);
	uint16_t pending = (uint16_t)(1u << RETRO_SNES_Y);
	EXPECT_FALSE(retroSnesData(shift));
	for (int i = 0; i < 3; i++)
		retroSnesClock(shift);

	retroSnesLatch(shift, pending);
	EXPECT_TRUE(retroSnesData(shift));
	retroSnesClock(shift);
	EXPECT_FALSE(retroSnesData(shift));

	for (int i = 0; i < 15; i++)
		retroSnesClock(shift);
	EXPECT_TRUE(retroSnesData(shift));
}

static uint32_t nextRandom(uint32_t &state)
{
	state = state * 1664525u + 1013904223u;
	return state;
}

static uint16_t expectedPressed(uint32_t buttons)
{
	uint16_t pressed = 0;
	for (uint8_t i = 0; i < RETRO_SNES_BUTTON_COUNT; i++)
		if (buttons & RETRO_SNES_DEFAULT_MAPPING.source[i])
			pressed |= (uint16_t)(1u << i);

	uint16_t vertical =
		(uint16_t)((1u << RETRO_SNES_UP) | (1u << RETRO_SNES_DOWN));
	uint16_t horizontal =
		(uint16_t)((1u << RETRO_SNES_LEFT) | (1u << RETRO_SNES_RIGHT));
	if ((pressed & vertical) == vertical)
		pressed &= (uint16_t)~vertical;
	if ((pressed & horizontal) == horizontal)
		pressed &= (uint16_t)~horizontal;
	return pressed;
}

static void testHighIterationSimulation()
{
	uint32_t random = 0xC001D00Du;
	RetroSnesShift shift;

	for (uint32_t iteration = 0; iteration < 1000000u; iteration++) {
		uint32_t buttons = nextRandom(random);
		uint16_t expected = expectedPressed(buttons);
		uint16_t pressed = retroSnesPressed(buttons, true,
						    RETRO_SNES_DEFAULT_MAPPING);
		EXPECT_EQ(pressed, expected);

		uint16_t wire = retroSnesWireWord(pressed);
		EXPECT_EQ(wire & 0xF000u, 0xF000u);
		retroSnesLatch(shift, pressed);
		for (uint8_t bit = 0; bit < 16; bit++) {
			EXPECT_EQ(retroSnesData(shift), (wire >> bit) & 1u);
			retroSnesClock(shift);
		}
		EXPECT_TRUE(retroSnesData(shift));
		if (g_failures)
			return;
	}
}

int main()
{
	testMapping();
	testMappingSafety();
	testShiftOrderAndCompletion();
	testLatchSnapshotAndRestart();
	testHighIterationSimulation();

	if (g_failures) {
		fprintf(stderr, "%d Retro SNES test failure(s)\n", g_failures);
		return 1;
	}
	printf("Retro SNES tests passed (1,000,000 simulated frames).\n");
	return 0;
}
