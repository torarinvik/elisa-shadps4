#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LEDGER_PATH = ROOT / "parity_ledger.json"


def fail(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


def load_lines(path: Path) -> list[str]:
    if not path.exists():
        raise FileNotFoundError(path)
    return path.read_text().splitlines()


def has_c_abi_before(lines: list[str], line_idx: int) -> bool:
    i = line_idx - 1
    while i >= 0:
        raw = lines[i].strip()
        if raw == "":
            i -= 1
            continue
        if raw.startswith("#"):
            i -= 1
            continue
        return raw == "@c_abi(c)"
    return False


def has_fixed_layout_for_struct(lines: list[str], struct_name: str) -> bool:
    pattern = re.compile(rf"^\s*struct\s+{re.escape(struct_name)}\s*:\s*$")
    for idx, line in enumerate(lines):
        if pattern.match(line):
            j = idx - 1
            while j >= 0:
                raw = lines[j].strip()
                if raw == "":
                    j -= 1
                    continue
                if raw.startswith("#"):
                    j -= 1
                    continue
                return raw == "@fixed_layout"
            return False
    return False


def extern_name(line: str) -> str | None:
    m = re.match(r"^\s*extern\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
    if not m:
        return None
    return m.group(1)


def check_external_entry(entry: dict) -> list[str]:
    errs: list[str] = []
    elisa_entry = entry.get("elisa_entry", "")
    symbol = entry.get("symbol", "")
    if "::" in elisa_entry:
        file_part, _ = elisa_entry.split("::", 1)
    else:
        file_part = elisa_entry
    target_file = ROOT / file_part
    try:
        lines = load_lines(target_file)
    except FileNotFoundError:
        return [f"{symbol}: missing elisa file for entry: {target_file}"]

    wildcard = symbol.endswith("*")
    if wildcard:
        prefix = symbol[:-1]
        matched = 0
        for i, line in enumerate(lines):
            name = extern_name(line)
            if name is None or not name.startswith(prefix):
                continue
            matched += 1
            if not has_c_abi_before(lines, i):
                errs.append(f"{symbol}: extern `{name}` missing @c_abi(c) in {file_part}")
        if matched == 0:
            errs.append(f"{symbol}: no externs matched prefix in {file_part}")
    else:
        found = False
        for i, line in enumerate(lines):
            name = extern_name(line)
            if name != symbol:
                continue
            found = True
            if not has_c_abi_before(lines, i):
                errs.append(f"{symbol}: missing @c_abi(c) in {file_part}")
        if not found:
            errs.append(f"{symbol}: extern not found in {file_part}")

    if "imgui_abi.elisa" in file_part:
        for struct_name in ("ElisaImGuiVec2", "ElisaImGuiVec4"):
            if not has_fixed_layout_for_struct(lines, struct_name):
                errs.append(f"{symbol}: `{struct_name}` missing @fixed_layout in {file_part}")

    if "memory_map.elisa" in file_part:
        if not has_fixed_layout_for_struct(lines, "Devtools_Widget_MemoryMapSnapshotRow"):
            errs.append(f"{symbol}: `Devtools_Widget_MemoryMapSnapshotRow` missing @fixed_layout in {file_part}")

    return errs


def main() -> int:
    if not LEDGER_PATH.exists():
        return fail(f"missing ledger: {LEDGER_PATH}")
    raw = json.loads(LEDGER_PATH.read_text())
    active_targets = raw.get("active_targets", [])
    if not active_targets:
        return fail("parity ledger has no active_targets")

    errors: list[str] = []
    checked = 0
    for target in active_targets:
        for entry in target.get("entries", []):
            if entry.get("status") != "External-C-ABI":
                continue
            checked += 1
            errors.extend(check_external_entry(entry))

    if errors:
        for e in errors:
            print(e, file=sys.stderr)
        return 1

    # Companion ABI-shape guard: these structs cross C/C++-defined library surfaces.
    companion_httpd = ROOT / "core/libraries/companion/companion_httpd.elisa"
    companion_util = ROOT / "core/libraries/companion/companion_util.elisa"
    if companion_httpd.exists():
        lines = load_lines(companion_httpd)
        for struct_name in (
            "OrbisCompanionHttpdHeader",
            "OrbisCompanionHttpdRequest",
            "OrbisCompanionHttpdResponse",
            "OrbisCompanionUtilDeviceInfo",
            "OrbisCompanionHttpdEvent",
        ):
            if not has_fixed_layout_for_struct(lines, struct_name):
                print(
                    f"Companion ABI: `{struct_name}` missing @fixed_layout in core/libraries/companion/companion_httpd.elisa",
                    file=sys.stderr,
                )
                return 1
    if companion_util.exists():
        lines = load_lines(companion_util)
        for struct_name in ("sceCompanionUtilEvent", "sceCompanionUtilContext"):
            if not has_fixed_layout_for_struct(lines, struct_name):
                print(
                    f"Companion ABI: `{struct_name}` missing @fixed_layout in core/libraries/companion/companion_util.elisa",
                    file=sys.stderr,
                )
                return 1

    if "--summary" in sys.argv:
        print(f"external c-abi entries checked: {checked}")
        print("abi guard: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
