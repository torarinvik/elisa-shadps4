#!/usr/bin/env python3
from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"


@dataclass(frozen=True)
class AbiCheck:
    name: str
    source: str
    extra_cppflags: tuple[str, ...] = ()
    optional: bool = False


def default_target_triple() -> str:
    machine = platform.machine().lower()
    if machine in {"amd64", "x86_64"}:
        arch = "x86_64"
    elif machine in {"arm64", "aarch64"}:
        arch = "aarch64"
    else:
        arch = machine or "unknown"
    if sys.platform == "darwin":
        return f"{arch}-apple-darwin"
    if sys.platform.startswith("linux"):
        return f"{arch}-unknown-linux-gnu"
    if sys.platform == "win32":
        return f"{arch}-pc-windows-msvc"
    return f"{arch}-unknown-unknown"


def ffmpeg_include_flags() -> tuple[str, ...]:
    candidates = [
        Path("/opt/homebrew/Cellar/ffmpeg/8.1.1/include"),
        Path("/opt/homebrew/include"),
        Path("/usr/local/include"),
        Path("/usr/include"),
    ]
    return tuple(f"-I{path}" for path in candidates if path.exists())


CHECKS = [
    AbiCheck("network sockets", "core/libraries/network/net_ffi.elisa"),
    AbiCheck("pad bridge DTOs", "core/libraries/pad/pad_ffi.elisa"),
    AbiCheck("voice bridge DTOs", "core/libraries/voice/voice_ffi.elisa"),
    AbiCheck("videodec bridge DTOs", "core/libraries/videodec/videodec_ffi.elisa"),
    AbiCheck("avplayer ffmpeg public prefixes", "core/libraries/avplayer/avplayer_ffmpeg_ffi.elisa", extra_cppflags=ffmpeg_include_flags(), optional=True),
]


def run_check(check: AbiCheck, target_triple: str) -> tuple[bool, str]:
    go = shutil.which("go")
    if go is None:
        return False, "go missing"
    source = ROOT / check.source
    if not source.exists():
        return False, f"source missing: {check.source}"
    env = os.environ.copy()
    # Keep the project include path relative to the compiler cwd so paths with
    # spaces do not get split by CPPFLAGS parsing.
    cppflags = ["-I../elisa-shad-ps4-from-scratch"]
    cppflags.extend(check.extra_cppflags)
    if env.get("CPPFLAGS"):
        cppflags.append(env["CPPFLAGS"])
    env["CPPFLAGS"] = " ".join(cppflags)
    cmd = [
        go,
        "run",
        "./src",
        "-emit",
        "c-bind-check",
        "-target-triple",
        target_triple,
        str(source),
    ]
    proc = subprocess.run(cmd, cwd=COMPILER_DIR, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    return proc.returncode == 0, proc.stdout.strip()


def main() -> int:
    target_triple = sys.argv[1] if len(sys.argv) > 1 else default_target_triple()
    failures: list[str] = []
    for check in CHECKS:
        ok, output = run_check(check, target_triple)
        status = "PASS" if ok else ("SKIP" if check.optional else "FAIL")
        print(f"EMULATOR_ABI_SMOKE {status} name={check.name!r} target={target_triple}")
        if output:
            for line in output.splitlines():
                if "c-bind-check:" in line or "error:" in line:
                    print(f"  {line}")
        if not ok and not check.optional:
            failures.append(check.name)
    if failures:
        print("EMULATOR_ABI_SMOKE status=failed failures=" + ",".join(failures))
        return 1
    print("EMULATOR_ABI_SMOKE status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
