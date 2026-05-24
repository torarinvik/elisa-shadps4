#!/usr/bin/env python3
from __future__ import annotations

import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"


def target_triple_for_x64_host() -> str:
    if sys.platform == "darwin":
        return "x86_64-apple-darwin"
    if sys.platform.startswith("linux"):
        return "x86_64-unknown-linux-gnu"
    if sys.platform == "win32":
        return "x86_64-pc-windows-msvc"
    return "x86_64-unknown-unknown"


def run(cmd: list[str], cwd: Path, timeout: int = 180) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)


def run_guest_exec_test(target_triple: str, filter_name: str, step: str) -> subprocess.CompletedProcess[str]:
    test_cmd = [
        "go",
        "run",
        "./src",
        "test",
        "emulator-guest-exec-runtime-tests",
        "--project",
        "../elisa-shad-ps4-from-scratch",
        "-target-triple",
        target_triple,
        "-filter",
        filter_name,
    ]
    result = run(test_cmd, COMPILER_DIR)
    if result.returncode != 0:
        print(f"GUEST_EXEC_ABI_SMOKE status=failed step={step} target={target_triple}")
        print(result.stdout, end="")
    return result


def is_rosetta_x64_target(target_triple: str) -> bool:
    machine = platform.machine().lower()
    return sys.platform == "darwin" and machine in {"arm64", "aarch64"} and target_triple.startswith("x86_64-")


def main() -> int:
    target_triple = sys.argv[1] if len(sys.argv) > 1 else target_triple_for_x64_host()
    entry_params_result = run_guest_exec_test(target_triple, "entry_params", "entry-params-layout")
    if entry_params_result.returncode != 0:
        return 1
    if is_rosetta_x64_target(target_triple):
        trampoline_result = None
        print(f"GUEST_EXEC_ABI_SMOKE guarded-trampoline=skipped target={target_triple} reason=rosetta-fork-signal-limit")
    else:
        trampoline_result = run_guest_exec_test(
            target_triple, "guarded_main_uses_guest_trampoline_abi", "guest-trampoline-abi"
        )
        if trampoline_result.returncode != 0:
            return 1

    print("GUEST_EXEC_ABI_SMOKE status=ok target=" + target_triple)
    combined_output = entry_params_result.stdout
    if trampoline_result is not None:
        combined_output += trampoline_result.stdout
    for line in combined_output.splitlines():
        if (
            "emulator_guest_exec_entry_params_abi_matches_native_layout" in line
            or "emulator_guest_exec_guarded_main_uses_guest_trampoline_abi" in line
            or "SUMMARY" in line
        ):
            print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
