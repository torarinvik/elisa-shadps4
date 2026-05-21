#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = ROOT / "core" / "libraries"
OUTPUT = LIBRARIES / "generated_hle_nids.elisa"
LIB_FUNCTION_RE = re.compile(
    r'LIB_FUNCTION\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*([^,]+)\s*,\s*"([^"]*)"\s*,\s*([^\)\n]+)\)'
)


def quote(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    dedicated_nid_dirs = {path.parent for path in LIBRARIES.rglob("*_nids.elisa")}
    entries: list[tuple[str, str, str, str, str]] = []

    for path in sorted(LIBRARIES.rglob("*.elisa")):
        if path.name.endswith("_nids.elisa") or path.name.endswith(".lowered.elisa"):
            continue
        if path.parent in dedicated_nid_dirs:
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        relpath = path.relative_to(ROOT).as_posix()
        for match in LIB_FUNCTION_RE.finditer(text):
            nid, lib_name, _version, export_lib, symbol_name = match.groups()
            entries.append((nid, lib_name, export_lib, symbol_name.strip(), relpath))

    with OUTPUT.open("w", encoding="utf-8") as out:
        out.write("# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project\n")
        out.write("# SPDX-License-Identifier: GPL-2.0-or-later\n\n")
        out.write("# Generated from remaining LIB_FUNCTION registrations that do not yet have dedicated *_nids.elisa tables.\n")
        out.write("# Regenerate with: python3 tools/generate_hle_nids.py\n\n")
        out.write("struct GeneratedHleNidEntry:\n")
        out.write("    nid: static u8&\n")
        out.write("    lib_name: static u8&\n")
        out.write("    export_lib: static u8&\n")
        out.write("    symbol_name: static u8&\n")
        out.write("    source_file: static u8&\n\n")
        out.write(f"const GENERATED_HLE_NIDS_COUNT: usize = {len(entries)}\n\n")
        out.write("def GeneratedHle_NidEntryAt(index: usize) -> GeneratedHleNidEntry:\n")
        for index, (nid, lib_name, export_lib, symbol_name, source_file) in enumerate(entries):
            out.write(f"    if index == {index}:\n")
            out.write(
                "        return GeneratedHleNidEntry("
                f'"{quote(nid)}", "{quote(lib_name)}", "{quote(export_lib)}", '
                f'"{quote(symbol_name)}", "{quote(source_file)}")\n'
            )
        out.write('    return GeneratedHleNidEntry("", "", "", "", "")\n')

    print(f"Wrote {OUTPUT.relative_to(ROOT)} with {len(entries)} entries")


if __name__ == "__main__":
    main()
