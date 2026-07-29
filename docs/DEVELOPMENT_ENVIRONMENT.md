# Development environment

OpenPuck's authoritative firmware path is Arduino CLI through the existing
Makefile. RetroPuck adds a repo-local toolchain convention so builds do not
overwrite global Arduino configuration.

## Quick start

From the repository root:

```sh
make setup
make doctor
make check
make build
```

`make setup` downloads only into ignored repository directories. It does not
run a privileged package-manager command:

- `.tools/` contains the pinned Arduino CLI and its config wrapper.
- `.venv/` contains clang-format 18 and adafruit-nrfutil.
- `.arduino/` contains the Arduino package index, core, compiler, cache, and
  user directory.

The setup script prints optional host package commands but never runs them.
It supports x86_64 and ARM64 Linux, and x86_64 and ARM64 macOS portable Arduino
archives. Arch Linux is the first tested host. Debian/Ubuntu and macOS paths
remain subject to CI or host verification.

Use `tools/run <command>` when a command outside Make needs the local tools:

```sh
tools/run arduino-cli core list
tools/run adafruit-nrfutil version
tools/run clang-format --version
```

## Pinned inputs

`tools/versions.env` is the human-readable version manifest.
`tools/requirements-dev.lock` pins the Python dependency graph with hashes.
The current inputs are:

| Tool | Version |
| --- | --- |
| Arduino CLI | 1.5.1 |
| Adafruit nRF52 core | 1.7.0 |
| adafruit-nrfutil | 0.5.3.post16 |
| clang-format | 18.1.8 |
| helper Python | 3.12 |
| Node.js for Retro Studio | 24.18.0 LTS |
| firmware FQBN | `adafruit:nrf52:feather52840` |

`.nvmrc` records the Studio Node version. Studio dependencies and their package
manager lock will be added with Studio rather than installing an empty frontend
workspace during firmware phases.

## Doctor behavior

`make doctor` is read-only. It reports:

- resolved executable paths and versions;
- the exact clang-format major;
- Arduino CLI, Adafruit core, and configured FQBN availability;
- adafruit-nrfutil and the host C++ compiler;
- later-phase Node.js and KiCad readiness;
- serial boards reported by Arduino CLI.

It never chooses a serial target. Missing firmware build dependencies cause a
nonzero exit. Later-phase dependencies are warnings until their phase begins.
On Linux it also reports stable `/dev/serial/by-id` links and metadata from
already-mounted UF2 volumes. It does not mount, reset, or flash a device.

## Local Arduino configuration

`arduino-cli.yaml` points Arduino at `.arduino/` and contains only the official
Adafruit board-manager URL. The ignored executable wrapper injects this config
path so Make's existing `arduino-cli` recipe remains unchanged. No global
Arduino config is created or overwritten.

## Clean rebuild with artifacts

To preserve build outputs in predictable paths:

```sh
make build \
  BUILD_PATH=build/cache/openpuck \
  OUTPUT_DIR=build/openpuck
```

Build outputs are ignored. Record artifact hashes in an implementation or
release manifest before cleanup.

## Host package prerequisites

The portable setup still expects basic host tools: Git, GNU Make, `curl`,
`tar`, `sha256sum`, a C++ compiler, and `uv`. On Arch Linux:

```sh
sudo pacman -S --needed base-devel curl git make uv
```

Review privileged commands before running them. The bootstrap deliberately
does not invoke `sudo` or a package manager.

## Windows follow-up

Native PowerShell bootstrap remains a scoped Phase 1 follow-up. It must mirror
the same pinned manifest, verify the official Windows Arduino CLI archive,
create an isolated Python environment, generate a config-injecting command
wrapper, and install the Adafruit core without changing the user's global
Arduino configuration. Until that path is implemented and tested on Windows,
use a supported Linux host or WSL and do not claim native Windows setup support.
