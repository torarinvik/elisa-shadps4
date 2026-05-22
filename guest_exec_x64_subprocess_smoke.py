#!/usr/bin/env python3
from __future__ import annotations

import os
import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TMP_DIR = Path(os.environ.get("TMPDIR", "/tmp"))
SOURCE = ROOT / "native" / "guest_exec_runtime.c"
OUTPUT = TMP_DIR / "elisa_guest_exec_x64_standalone"
HANDOFF_MANIFEST = ROOT / "cusa07399_handoff_manifest.txt"


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

def parse_kv_line(line: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for token in line.strip().split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        out[key] = value
    return out


def to_int(value: str | None) -> int:
    if value is None:
        return 0
    try:
        return int(value, 16 if value.lower().startswith("0x") else 10)
    except ValueError:
        return 0


def validate_handoff_manifest() -> int:
    if not HANDOFF_MANIFEST.exists():
        print(f"CUSA07399_HANDOFF_X64_MANIFEST status=missing path={HANDOFF_MANIFEST}")
        return 1
    lines = [line.strip() for line in HANDOFF_MANIFEST.read_text().splitlines() if line.strip()]
    summary_lines = [line for line in lines if line.startswith("CUSA07399_HANDOFF_MANIFEST")]
    module_lines = [line for line in lines if line.startswith("CUSA07399_HANDOFF_MODULE")]
    if not summary_lines:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=missing-summary")
        return 1
    summary = parse_kv_line(summary_lines[-1])
    module_count = to_int(summary.get("module_count"))
    entry = to_int(summary.get("entry"))
    main_entry = to_int(summary.get("main_entry"))
    base = to_int(summary.get("base"))
    image_size = to_int(summary.get("image_size"))
    executable_segments = to_int(summary.get("executable_segments"))
    imports = to_int(summary.get("imports"))
    resolved_imports = to_int(summary.get("resolved_imports"))
    patched_import_relocations = to_int(summary.get("patched_import_relocations"))
    if module_count <= 0 or len(module_lines) <= 0:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=empty-modules")
        return 1
    if entry == 0 or main_entry == 0 or base == 0 or image_size == 0:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=invalid-addresses")
        return 1
    if entry != main_entry:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=entry-mismatch")
        return 1
    if not (base <= entry < base + image_size):
        print("CUSA07399_HANDOFF_X64_MANIFEST status=entry-outside-image")
        return 1
    if executable_segments <= 0:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=no-executable-segments")
        return 1
    if imports <= 0 or resolved_imports != imports:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=import-resolution-mismatch")
        return 1
    if patched_import_relocations <= 0:
        print("CUSA07399_HANDOFF_X64_MANIFEST status=no-patched-import-relocations")
        return 1
    print(
        "CUSA07399_HANDOFF_X64_MANIFEST "
        f"status=ok module_count={module_count} entry=0x{entry:x} "
        f"base=0x{base:x} image_size=0x{image_size:x} "
        f"executable_segments={executable_segments} imports={imports}"
    )
    return 0


def main() -> int:
    system = platform.system()
    machine = platform.machine().lower()
    compile_cmd = [
        "clang",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DELISA_GUEST_EXEC_STANDALONE_TEST",
        str(SOURCE),
        "-o",
        str(OUTPUT),
    ]
    run_cmd = [str(OUTPUT)]
    if system == "Darwin" and machine in {"arm64", "aarch64"}:
        compile_cmd.insert(1, "-arch")
        compile_cmd.insert(2, "x86_64")
        run_cmd = ["/usr/bin/arch", "-x86_64", str(OUTPUT)]
    elif machine not in {"x86_64", "amd64"}:
        print(f"SKIP guest-exec x64 subprocess smoke: unsupported host {system} {machine}")
        return 0

    compiled = run(compile_cmd)
    if compiled.returncode != 0:
        print(compiled.stdout, end="")
        return compiled.returncode

    executed = run(run_cmd)
    print(executed.stdout, end="")
    if executed.returncode != 0:
        return executed.returncode
    if "ELISA_GUEST_EXEC_X64_STANDALONE_OK" not in executed.stdout:
        print("missing x64 standalone success marker")
        return 1
    return validate_handoff_manifest()


if __name__ == "__main__":
    raise SystemExit(main())
