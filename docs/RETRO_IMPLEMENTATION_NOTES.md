# Retro implementation notes

## Baseline

- Upstream: `https://github.com/safijari/openpuck.git`
- Fork: `https://github.com/Seglectic/openpuck`
- Feature branch: `retropuck`
- Base SHA: `75b57ca8cf71049457d77efb0f9bd6937cbdbd49`
- Base commit time: `2026-07-27T23:46:22-05:00`
- Baseline capture time: `2026-07-29T02:23:43Z`
- Host: Arch Linux x86_64, kernel `7.1.3-arch2-2`
- Board FQBN: `adafruit:nrf52:feather52840`

## Tool snapshot

| Tool | Version |
| --- | --- |
| Git | 2.55.0 |
| GitHub CLI | 2.96.0 |
| GNU Make | 4.4.1 |
| helper Python | 3.12.13 |
| host C++ | GCC 16.1.1 |
| Arduino CLI | 1.5.1 |
| Adafruit nRF52 core | 1.7.0 |
| adafruit-nrfutil | 0.5.3.post16 |
| clang-format | 18.1.8 |

Node.js 26.4.0 is installed globally but is not the pinned Studio runtime.
Node.js 24.18.0 LTS is recorded in `.nvmrc`. `kicad-cli` is not installed and
remains a Phase 7 gate.

## Untouched firmware baseline

The first `make check` attempt occurred before the pinned formatter was
installed. With an empty `CLANG_FORMAT`, the upstream recipe's leading
`--dry-run` was interpreted by Make as an ignored command, producing an invalid
apparent pass. No source was changed in response.

After local setup:

```sh
make check
make build \
  BUILD_PATH=build/cache/baseline \
  OUTPUT_DIR=build/baseline
```

Results:

- format check: pass with clang-format 18.1.8;
- compile: pass in 9.193 seconds;
- flash: 187,184 of 815,104 bytes (22%);
- RAM: 42,512 of 237,568 bytes (17%);
- assembler emitted the existing
  `.data.opk_ramfunc` section-attribute warning.

Artifacts:

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `OpenPuck.ino.elf` | 3,552,260 | `ae73e30fc64c71352bdee16ce85be5b02e14c1123e61f7dd379800cb750da740` |
| `OpenPuck.ino.hex` | 528,915 | `a0703cbb71a453308ae169cc2b937ca95d22a8bdccadd079804a721d8a225298` |
| `OpenPuck.ino.map` | 1,479,056 | `75aa9b16998fd84d8cd22704c827ea81f7205adb87d8e6e065b1ea3cff8280ef` |
| `OpenPuck.ino.zip` | 188,838 | `844f384ad54bf4a175172c7ababee5a92538d59d57fc94a4f76ee5725639410c` |

No hardware or serial target was present, selected, or flashed.

## Integration strategy

Retro output is a build-selected backend, not a USB personality. The stock
build will select a null backend and configure no retro GPIO.

The intended dependency direction is:

```text
RF decode -> g_in[slot] -> backend lifecycle -> pure SNES model
                                         \-> SNES GPIO adapter
```

The integration diff in existing firmware should be limited to backend
lifecycle calls. SNES mapping, state, GPIO, configuration, and tests belong in
new single-purpose modules. Existing `mode_*.cpp` files remain unaware of retro
support.

Retro Studio consumes documented commands, schemas, manifests, and WebUSB
capabilities. Firmware does not depend on Studio. Carrier hardware consumes a
physical pin and power contract rather than firmware source structure.

At each phase boundary, fetch `upstream/main`, record its SHA, inspect conflict
surfaces, and rerun the stock baseline before integrating upstream changes.
