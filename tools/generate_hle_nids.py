#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = ROOT / "core" / "libraries"
AGGREGATE_OUTPUT = LIBRARIES / "generated_hle_nids.elisa"
LIB_FUNCTION_RE = re.compile(
    r'LIB_FUNCTION\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*([^,]+)\s*,\s*"([^"]*)"\s*,\s*([^\)\n]+)\)'
)


def quote(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def table_name(path: Path) -> str:
    name = path.name
    if name == "save_data":
        name = "savedata"
    return name


def type_prefix(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_"))


def const_prefix(name: str) -> str:
    return name.upper()


def write_table(output: Path, name: str, entries: list[tuple[str, str, str, str, str]]) -> None:
    prefix = type_prefix(name)
    constant = const_prefix(name)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as out:
        out.write("# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project\n")
        out.write("# SPDX-License-Identifier: GPL-2.0-or-later\n\n")
        out.write("# Generated from LIB_FUNCTION registrations owned by this library module.\n")
        out.write("# Regenerate with: python3 tools/generate_hle_nids.py\n\n")
        out.write(f"struct {prefix}NidEntry:\n")
        out.write("    nid: static u8&\n")
        out.write("    lib_name: static u8&\n")
        out.write("    export_lib: static u8&\n")
        out.write("    symbol_name: static u8&\n")
        out.write("    source_file: static u8&\n\n")
        out.write(f"const {constant}_NIDS_COUNT: usize = {len(entries)}\n\n")
        out.write(f"def {prefix}_NidEntryAt(index: usize) -> {prefix}NidEntry:\n")
        for index, (nid, lib_name, export_lib, symbol_name, source_file) in enumerate(entries):
            out.write(f"    if index == {index}usize:\n")
            out.write(
                f"        return {prefix}NidEntry("
                f'"{quote(nid)}", "{quote(lib_name)}", "{quote(export_lib)}", '
                f'"{quote(symbol_name)}", "{quote(source_file)}")\n'
            )
        out.write(f'    return {prefix}NidEntry("", "", "", "", "")\n')


def main() -> None:
    dedicated_nid_dirs = {
        path.parent
        for path in LIBRARIES.rglob("*_nids.elisa")
        if "Generated from LIB_FUNCTION registrations owned by this library module."
        not in path.read_text(encoding="utf-8", errors="ignore")
    }
    entries_by_module: dict[Path, list[tuple[str, str, str, str, str]]] = {}

    for path in sorted(LIBRARIES.rglob("*.elisa")):
        if path.name.endswith("_nids.elisa") or path.name.endswith(".lowered.elisa"):
            continue
        if path.parent in dedicated_nid_dirs:
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        relpath = path.relative_to(ROOT).as_posix()
        for match in LIB_FUNCTION_RE.finditer(text):
            nid, lib_name, _version, export_lib, symbol_name = match.groups()
            module_dir = path.relative_to(LIBRARIES).parts[0]
            entries_by_module.setdefault(LIBRARIES / module_dir, []).append(
                (nid, lib_name, export_lib, symbol_name.strip(), relpath)
            )

    generated_modules: list[tuple[str, Path, int]] = []
    for module_path, entries in sorted(entries_by_module.items(), key=lambda item: item[0].as_posix()):
        name = table_name(module_path)
        output = module_path / f"{name}_nids.elisa"
        write_table(output, name, entries)
        generated_modules.append((name, output, len(entries)))

    with AGGREGATE_OUTPUT.open("w", encoding="utf-8") as out:
        out.write("# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project\n")
        out.write("# SPDX-License-Identifier: GPL-2.0-or-later\n\n")
        out.write("# Generated aggregate for per-library fallback NID tables.\n")
        out.write("# Regenerate with: python3 tools/generate_hle_nids.py\n\n")
        for _name, output, _count in generated_modules:
            rel = output.relative_to(LIBRARIES).as_posix()
            out.write(f'include "./{rel}"\n')
        out.write("\n")
        for name, _output, _count in generated_modules:
            prefix = type_prefix(name)
            constant = const_prefix(name)
            out.write(f"def HLELibs_Register{prefix}GeneratedSymbols(sym: mutable SymbolsResolver&) -> void:\n")
            out.write(f"    for i in 0..<{constant}_NIDS_COUNT:\n")
            out.write(f"        entry: {prefix}NidEntry = {prefix}_NidEntryAt(i)\n")
            out.write("        HLELibs_RegisterSymbol(sym, entry.nid, entry.symbol_name)\n\n")
        out.write("def HLELibs_RegisterGeneratedSymbols(sym: mutable SymbolsResolver&) -> void:\n")
        for name, _output, _count in generated_modules:
            out.write(f"    HLELibs_Register{type_prefix(name)}GeneratedSymbols(sym)\n")

    total_entries = sum(count for _name, _output, count in generated_modules)
    print(f"Wrote {len(generated_modules)} per-library NID tables with {total_entries} entries")


if __name__ == "__main__":
    main()
