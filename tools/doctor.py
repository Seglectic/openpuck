#!/usr/bin/env python3
# ╭────────────────────────────────────────╮
# │  RetroPuck Development Doctor          │
# │  Audits local build tools, board setup,│
# │  and serial targets without mutation.  │
# ╰────────────────────────────────────────╯

from __future__ import annotations

import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
VERSIONS_FILE = ROOT / "tools" / "versions.env"
ARDUINO_CONFIG = ROOT / "arduino-cli.yaml"


@dataclass(frozen=True)
class Result:
    level: str
    name: str
    found: str
    required: str
    remediation: str = ""


def load_versions() -> dict[str, str]:
    versions: dict[str, str] = {}
    for raw in VERSIONS_FILE.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        key, value = line.split("=", 1)
        versions[key] = value
    return versions


def command_path(name: str, local: Path | None = None) -> str | None:
    if local and local.is_file():
        return str(local)
    return shutil.which(name)


def run(command: list[str], timeout: int = 20) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        return subprocess.CompletedProcess(command, 127, str(error))


def first_line(command: list[str]) -> tuple[int, str]:
    completed = run(command)
    output = completed.stdout.strip().splitlines()
    return completed.returncode, output[0] if output else "no version output"


def numeric_version(text: str) -> tuple[int, ...]:
    match = re.search(r"\d+(?:\.\d+)+", text)
    if not match:
        return ()
    return tuple(int(part) for part in match.group(0).split("."))


def executable_check(
    name: str,
    executable: str | None,
    args: list[str],
    requirement: str,
    predicate,
    remediation: str,
) -> Result:
    if not executable:
        return Result("FAIL", name, "not found", requirement, remediation)
    code, output = first_line([executable, *args])
    found = f"{executable}: {output}"
    if code != 0 or not predicate(output):
        return Result("FAIL", name, found, requirement, remediation)
    return Result("PASS", name, found, requirement)


def optional_check(
    name: str,
    executable: str | None,
    args: list[str],
    requirement: str,
    predicate,
    remediation: str,
) -> Result:
    result = executable_check(
        name, executable, args, requirement, predicate, remediation
    )
    if result.level == "FAIL":
        return Result(
            "WARN",
            result.name,
            result.found,
            result.required,
            result.remediation,
        )
    return result


def arduino_checks(
    executable: str | None, versions: dict[str, str]
) -> list[Result]:
    if not executable:
        return [
            Result(
                "FAIL",
                "Arduino CLI",
                "not found",
                versions["ARDUINO_CLI_VERSION"],
                "Run tools/bootstrap-dev.sh.",
            )
        ]

    config_args = [executable, "--config-file", str(ARDUINO_CONFIG)]
    code, output = first_line([executable, "version"])
    version = numeric_version(output)
    expected = numeric_version(versions["ARDUINO_CLI_VERSION"])
    results = [
        Result(
            "PASS" if code == 0 and version == expected else "FAIL",
            "Arduino CLI",
            f"{executable}: {output}",
            versions["ARDUINO_CLI_VERSION"],
            "" if version == expected else "Run tools/bootstrap-dev.sh.",
        )
    ]

    core = run([*config_args, "core", "list"])
    core_pattern = re.compile(
        rf"^adafruit:nrf52\s+{re.escape(versions['ADAFRUIT_NRF52_VERSION'])}\b",
        re.MULTILINE,
    )
    core_ok = core.returncode == 0 and core_pattern.search(core.stdout)
    results.append(
        Result(
            "PASS" if core_ok else "FAIL",
            "Adafruit nRF52 core",
            "installed at pinned version" if core_ok else core.stdout.strip(),
            versions["ADAFRUIT_NRF52_VERSION"],
            "" if core_ok else "Run tools/bootstrap-dev.sh.",
        )
    )

    fqbn = versions["FQBN"]
    boards = run([*config_args, "board", "listall", fqbn])
    fqbn_ok = boards.returncode == 0 and fqbn in boards.stdout
    results.append(
        Result(
            "PASS" if fqbn_ok else "FAIL",
            "Firmware board",
            fqbn if fqbn_ok else boards.stdout.strip(),
            fqbn,
            "" if fqbn_ok else "Install the pinned Adafruit nRF52 core.",
        )
    )
    return results


def serial_inventory(executable: str | None) -> None:
    print("\nSerial inventory (informational; no target is selected):")
    if not executable:
        print("  unavailable until Arduino CLI is installed")
        return
    completed = run(
        [executable, "--config-file", str(ARDUINO_CONFIG), "board", "list"]
    )
    if completed.returncode != 0:
        print(f"  unable to enumerate: {completed.stdout.strip()}")
        return
    lines = completed.stdout.strip().splitlines()
    if len(lines) <= 1:
        print("  no serial boards detected")
        return
    for line in lines:
        print(f"  {line}")


def main() -> int:
    versions = load_versions()
    local_bin = ROOT / ".tools" / "bin"
    venv_bin = ROOT / ".venv" / "bin"
    arduino = command_path("arduino-cli", local_bin / "arduino-cli-real")
    formatter = command_path("clang-format-18", venv_bin / "clang-format")
    nrfutil = command_path("adafruit-nrfutil", venv_bin / "adafruit-nrfutil")

    required = [
        executable_check(
            "Git",
            command_path("git"),
            ["--version"],
            "available",
            lambda _: True,
            "Install Git with the host package manager.",
        ),
        executable_check(
            "GNU Make",
            command_path("make"),
            ["--version"],
            "available",
            lambda text: "GNU Make" in text,
            "Install GNU Make with the host package manager.",
        ),
        executable_check(
            "Python",
            command_path("python3"),
            ["--version"],
            ">= 3.10",
            lambda text: numeric_version(text) >= (3, 10),
            "Install Python 3.10 or newer.",
        ),
        executable_check(
            "C++ compiler",
            command_path("c++"),
            ["--version"],
            "available",
            lambda _: True,
            "Install a host C++ compiler.",
        ),
        executable_check(
            "clang-format",
            formatter,
            ["--version"],
            "major 18",
            lambda text: numeric_version(text)[:1] == (18,),
            "Run tools/bootstrap-dev.sh.",
        ),
        executable_check(
            "adafruit-nrfutil",
            nrfutil,
            ["version"],
            versions["ADAFRUIT_NRFUTIL_VERSION"],
            lambda text: versions["ADAFRUIT_NRFUTIL_VERSION"] in text,
            "Run tools/bootstrap-dev.sh.",
        ),
        *arduino_checks(arduino, versions),
    ]

    node_major = numeric_version(versions["NODE_VERSION"])[0]
    future = [
        optional_check(
            "GitHub CLI",
            command_path("gh"),
            ["--version"],
            "authenticated when publishing",
            lambda _: True,
            "Install GitHub CLI before publishing.",
        ),
        optional_check(
            "Node.js",
            command_path("node"),
            ["--version"],
            f"major {node_major} active LTS",
            lambda text: numeric_version(text)[:1] == (node_major,),
            f"Install Node.js {versions['NODE_VERSION']} before Studio work.",
        ),
        optional_check(
            "kicad-cli",
            command_path("kicad-cli"),
            ["--version"],
            "required for Phase 7",
            lambda _: True,
            "Install KiCad CLI before carrier-board validation.",
        ),
    ]

    print("Required firmware build tools:")
    for result in required:
        print(f"[{result.level}] {result.name}")
        print(f"  found:    {result.found or 'not found'}")
        print(f"  required: {result.required}")
        if result.remediation:
            print(f"  fix:      {result.remediation}")

    print("\nLater-phase tools:")
    for result in future:
        print(f"[{result.level}] {result.name}")
        print(f"  found:    {result.found or 'not found'}")
        print(f"  required: {result.required}")
        if result.remediation:
            print(f"  fix:      {result.remediation}")

    serial_inventory(arduino)
    failures = sum(result.level == "FAIL" for result in required)
    print(f"\nDoctor result: {failures} required failure(s).")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
