# Retro output backends

Retro output is an optional consumer of the decoded controller input. It is not
a USB personality and must not add a USB mode, descriptor, VID/PID, or
SNES-specific branch to an existing `mode_*.cpp` module.

The intended input direction is:

```text
RF decode -> g_in[slot] -> optional retro backend -> protocol adapter
```

The stock build will use a null backend. Backend selection and lifecycle hooks
are Phase 3 work; the Phase 2 SNES model is deliberately independent of
Arduino, GPIO, USB, RF, allocation, and wall-clock time.

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
