#!/bin/sh
# ╭───────────────────────────────────────╮
# │  RetroPuck Development Bootstrap      │
# │  Installs pinned build tools into the │
# │  repository without global changes.   │
# ╰───────────────────────────────────────╯

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$ROOT/tools/versions.env"

TOOLS_DIR="$ROOT/.tools"
BIN_DIR="$TOOLS_DIR/bin"
VENV_DIR="$ROOT/.venv"
ARDUINO="$BIN_DIR/arduino-cli-real"
ARDUINO_WRAPPER="$BIN_DIR/arduino-cli"
CONFIG="$ROOT/arduino-cli.yaml"

info()
{
	printf '%s\n' "$*"
}

fail()
{
	printf 'error: %s\n' "$*" >&2
	exit 1
}

need()
{
	command -v "$1" >/dev/null 2>&1 || fail "required host tool not found: $1"
}

sha256_file()
{
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | awk '{ print $1 }'
	elif command -v shasum >/dev/null 2>&1; then
		shasum -a 256 "$1" | awk '{ print $1 }'
	else
		fail "sha256sum or shasum is required to verify downloads"
	fi
}

host_hint()
{
	if [ -r /etc/arch-release ]; then
		info "Host: Arch Linux."
		info "Optional system packages: sudo pacman -S --needed curl git make uv"
	elif [ -r /etc/debian_version ]; then
		info "Host: Debian/Ubuntu."
		info "Optional system packages: sudo apt install curl git make python3 uv"
	elif [ "$(uname -s)" = Darwin ]; then
		info "Host: macOS."
		info "Optional system packages: brew install curl git make uv"
	else
		info "Host: unsupported package manager; using portable local installs."
	fi
	info "No privileged command is run by this script."
}

arduino_asset()
{
	os=$(uname -s)
	arch=$(uname -m)

	case "$os:$arch" in
	Linux:x86_64)
		printf 'Linux_64bit.tar.gz\n'
		;;
	Linux:aarch64 | Linux:arm64)
		printf 'Linux_ARM64.tar.gz\n'
		;;
	Darwin:x86_64)
		printf 'macOS_64bit.tar.gz\n'
		;;
	Darwin:arm64)
		printf 'macOS_ARM64.tar.gz\n'
		;;
	*)
		fail "no pinned Arduino CLI archive for $os/$arch"
		;;
	esac
}

install_arduino_cli()
{
	if [ -x "$ARDUINO" ] &&
		"$ARDUINO" version | grep -q "$ARDUINO_CLI_VERSION"; then
		info "Arduino CLI $ARDUINO_CLI_VERSION already installed."
		return
	fi

	asset="arduino-cli_${ARDUINO_CLI_VERSION}_$(arduino_asset)"
	base="https://github.com/arduino/arduino-cli/releases/download"
	url="$base/v$ARDUINO_CLI_VERSION"
	tmp="$TOOLS_DIR/downloads/$ARDUINO_CLI_VERSION"

	mkdir -p "$tmp" "$BIN_DIR"
	info "Downloading Arduino CLI $ARDUINO_CLI_VERSION."
	curl -fL "$url/$asset" -o "$tmp/$asset"
	curl -fL "$url/${ARDUINO_CLI_VERSION}-checksums.txt" \
		-o "$tmp/checksums.txt"
	expected=$(awk -v file="$asset" '$2 == file { print $1 }' \
		"$tmp/checksums.txt")
	[ -n "$expected" ] || fail "Arduino CLI checksum is missing for $asset"
	actual=$(sha256_file "$tmp/$asset")
	[ "$actual" = "$expected" ] || fail "Arduino CLI checksum mismatch"
	tar -xzf "$tmp/$asset" -C "$BIN_DIR" arduino-cli
	mv "$BIN_DIR/arduino-cli" "$ARDUINO"
}

install_arduino_wrapper()
{
	{
		printf '%s\n' '#!/bin/sh'
		printf '%s\n' 'set -eu'
		printf '%s\n' \
			'ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)'
		printf '%s\n' \
			'exec "$ROOT/.tools/bin/arduino-cli-real" --config-file "$ROOT/arduino-cli.yaml" "$@"'
	} >"$ARDUINO_WRAPPER"
	chmod +x "$ARDUINO_WRAPPER"
}

install_python_tools()
{
	need uv

	if [ ! -x "$VENV_DIR/bin/python" ]; then
		info "Creating Python $HELPER_PYTHON_VERSION helper environment."
		uv venv --python "$HELPER_PYTHON_VERSION" "$VENV_DIR"
	fi
	info "Syncing pinned Python build tools."
	uv pip sync --python "$VENV_DIR/bin/python" \
		"$ROOT/tools/requirements-dev.lock"
}

install_arduino_core()
{
	export PATH="$BIN_DIR:$VENV_DIR/bin:$PATH"

	if "$ARDUINO" --config-file "$CONFIG" core list |
		awk -v version="$ADAFRUIT_NRF52_VERSION" \
			'$1 == "adafruit:nrf52" && $2 == version { found = 1 }
			END { exit !found }'; then
		info "Adafruit nRF52 core $ADAFRUIT_NRF52_VERSION already installed."
		return
	fi

	info "Updating the repo-local Arduino package index."
	"$ARDUINO" --config-file "$CONFIG" core update-index
	info "Installing Adafruit nRF52 core $ADAFRUIT_NRF52_VERSION."
	"$ARDUINO" --config-file "$CONFIG" core install \
		"adafruit:nrf52@$ADAFRUIT_NRF52_VERSION"
}

host_hint
need curl
need tar
install_arduino_cli
install_arduino_wrapper
install_python_tools
install_arduino_core

info
info "Local toolchain is installed. Verify with:"
info "  tools/run python3 tools/doctor.py"
