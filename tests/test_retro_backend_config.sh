#!/bin/sh
# ╭────────────────────────────────────────╮
# │  Retro Backend Compile-Time Tests      │
# │  Proves invalid selectors, slots, and  │
# │  physical pin profiles fail closed.    │
# ╰────────────────────────────────────────╯

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CXX=${CXX:-c++}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

printf '%s\n' '#include "retro_backend.h"' 'int main() { return 0; }' \
	>"$TMP/check.cpp"

expect_pass()
{
	name=$1
	shift
	if ! "$CXX" -std=c++17 -fsyntax-only -IOpenPuck "$@" \
		"$TMP/check.cpp" >"$TMP/$name.log" 2>&1; then
		printf 'expected compile pass: %s\n' "$name" >&2
		cat "$TMP/$name.log" >&2
		exit 1
	fi
}

expect_fail()
{
	name=$1
	shift
	if "$CXX" -std=c++17 -fsyntax-only -IOpenPuck "$@" \
		"$TMP/check.cpp" >"$TMP/$name.log" 2>&1; then
		printf 'expected compile failure: %s\n' "$name" >&2
		exit 1
	fi
}

cd "$ROOT"
expect_pass null
expect_pass nicenano -DOPK_RETRO_BACKEND=1 \
	-DOPK_RETRO_BOARD_NICENANO=1
expect_fail unknown_backend -DOPK_RETRO_BACKEND=2
expect_fail missing_pins -DOPK_RETRO_BACKEND=1
expect_fail duplicate_pins -DOPK_RETRO_BACKEND=1 \
	-DOPK_RETRO_LATCH_PIN=17 -DOPK_RETRO_CLOCK_PIN=17 \
	-DOPK_RETRO_DATA_PIN=22
expect_fail invalid_pin -DOPK_RETRO_BACKEND=1 \
	-DOPK_RETRO_LATCH_PIN=48 -DOPK_RETRO_CLOCK_PIN=20 \
	-DOPK_RETRO_DATA_PIN=22
expect_fail reserved_pin -DOPK_RETRO_BACKEND=1 \
	-DOPK_RETRO_BOARD_NICENANO=1 -DOPK_RETRO_LATCH_PIN=4
expect_fail invalid_slot -DOPK_RETRO_SLOT=4

printf '%s\n' "Retro backend compile-time tests passed."
