#!/usr/bin/env python3
from __future__ import annotations

import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"
TMP_OBJ = Path("/tmp/elisa_guest_exec_runtime_abi_smoke.o")


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


def main() -> int:
    target_triple = sys.argv[1] if len(sys.argv) > 1 else target_triple_for_x64_host()
    clang_cmd = ["clang", "-Wall", "-Wextra", "-Werror", "-pthread", "-c", "native/guest_exec_runtime.c", "-o", str(TMP_OBJ)]
    if platform.system() == "Darwin":
        clang_cmd.insert(1, "-DMAP_ANONYMOUS=MAP_ANON")
    clang_result = run(clang_cmd, ROOT)
    if clang_result.returncode != 0:
        print("GUEST_EXEC_ABI_SMOKE status=failed step=native-warning-build")
        print(clang_result.stdout, end="")
        return 1

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
        "entry_params",
    ]
    test_result = run(test_cmd, COMPILER_DIR)
    if test_result.returncode != 0:
        print("GUEST_EXEC_ABI_SMOKE status=failed step=entry-params-layout target=" + target_triple)
        print(test_result.stdout, end="")
        return 1
    print("GUEST_EXEC_ABI_SMOKE status=ok target=" + target_triple)
    for line in test_result.stdout.splitlines():
        if "emulator_guest_exec_entry_params_abi_matches_native_layout" in line or "SUMMARY" in line:
            print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
