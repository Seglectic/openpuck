# Retro output backends

Retro output is an optional consumer of the decoded controller input. It is not
a USB personality and must not add a USB mode, descriptor, VID/PID, or
SNES-specific branch to an existing `mode_*.cpp` module.

The intended input direction is:

```text
RF decode -> g_in[slot] -> optional retro backend -> protocol adapter
```

The stock build will use a null backend. Backend selection and lifecycle hooks
are owned by `retro_backend.*`; the Phase 2 SNES model remains independent of
Arduino, GPIO, USB, RF, allocation, and wall-clock time.

## Build selection

The compile-time selector accepts:

- `OPK_RETRO_NONE` (default): inline no-op lifecycle calls;
- `OPK_RETRO_SNES`: SNES pending-state preparation;
- `OPK_RETRO_SLOT` (default 0): selected bond slot.

The SNES selection requires explicit LATCH, CLOCK, and DATA physical nRF pin
identities encoded as P0.00–P0.31 = 0–31 and P1.00–P1.15 = 32–47. Compilation
rejects missing, duplicate, out-of-range, or board-reserved pins and an invalid
slot.

The reference nice!nano signal profile uses header pins common to the
[published v1 and v2 layouts](https://nicekeyboards.com/docs/nice-nano/pinout-schematic/):

| Function | Physical nRF pin | nice!nano label |
| --- | --- | --- |
| LATCH input | P0.17 | D2 |
| CLOCK input | P0.20 | D3 |
| DATA output | P0.22 | D4 |

These are logical MCU-side assignments only. They do not authorize a direct
connection to 5 V SNES signals. Level translation and backfeed protection remain
mandatory.

Build the selected profile with:

```sh
make build-snes \
  BUILD_PATH=build/cache/snes \
  OUTPUT_DIR=build/snes
```

The current Phase 3 backend prepares an RF-fresh logical word but intentionally
does not configure GPIO or report console activity. Phase 4 will add the
measured edge adapter; until then `retroBackendPollingActive()` remains false
and OpenPuck mode chords are not suppressed.

## SNES logical frame

`retroSnesPressed()` returns a logical 16-bit word where a set bit means the
button is pressed:

| Bit | Button | Default Triton source |
| ---: | --- | --- |
| 0 | B | `TB_B` |
| 1 | Y | `TB_Y` |
| 2 | Select | `TB_VIEW` |
| 3 | Start | `TB_MENU` |
| 4 | Up | `TB_DUP` |
| 5 | Down | `TB_DDN` |
| 6 | Left | `TB_DLF` |
| 7 | Right | `TB_DRT` |
| 8 | A | `TB_A` |
| 9 | X | `TB_X` |
| 10 | L | `TB_LB` |
| 11 | R | `TB_RB` |
| 12–15 | unused | always released |

Back paddles, QAM, stick clicks, trackpad clicks, and analog triggers are not
valid mapping sources in the initial model. A future configuration format can
expand the allowlist without changing the wire protocol.

Mappings are explicit and validated. Each nonzero source must be one allowed,
single controller bit and cannot be reused by another SNES button. Zero means
unmapped. An invalid mapping fails closed to all released.

Simultaneous Up+Down or Left+Right inputs are neutralized as pairs. Other
buttons in the same snapshot remain pressed.

## Active-low wire word

`retroSnesWireWord()` converts the logical pressed word to the SNES active-low
wire representation. Bits 12–15 are forced high. Tests and configuration use
the logical word so pressed/released assertions remain readable.

The bit order on DATA is:

```text
B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R, high, high, high, high
```

## Shift state

`RetroSnesShift` snapshots one logical word on each latch. DATA exposes the
current active-low bit, least significant bit first, and a clock advances the
position.

- A new latch replaces an incomplete frame.
- Changes to pending controller input do not alter a latched frame.
- After 16 clocks, DATA returns high.
- Incomplete and extra clock sequences remain bounded.
- Initialization and completion both expose released/high.

The state model contains no timing or GPIO operations. Phase 4 will translate
physical edges to these deterministic state transitions inside a bounded
hardware adapter.

## Native verification

Run:

```sh
make test-retro
```

The native test covers individual buttons, combinations, bit order,
active-low conversion, invalid mappings, RF-link loss, opposing directions,
latch snapshots, interrupted frames, extra clocks, completion safety, and one
million deterministic randomized frames.
