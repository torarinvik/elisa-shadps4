#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent

CPP_SPECS = {
    "audioout": ROOT / "core/libraries/audio/audioout.cpp",
    "audioin": ROOT / "core/libraries/audio/audioin.cpp",
    "pad": ROOT / "core/libraries/pad/pad.cpp",
}

ELISA_NID_FILES = {
    "audioout": ROOT / "core/libraries/audio/audioout_nids.elisa",
    "audioin": ROOT / "core/libraries/audio/audioin_nids.elisa",
}

ELISA_LIB_FILES = {
    "audioout": ROOT / "core/libraries/audio/audioout.elisa",
    "audioin": ROOT / "core/libraries/audio/audioin.elisa",
    "pad": ROOT / "core/libraries/pad/pad.elisa",
}

JSON_OUT = ROOT / "audio_input_symbol_nid_coverage.json"
MD_OUT = ROOT / "audio_input_symbol_nid_coverage.md"


def parse_cpp_nids(path: Path) -> set[str]:
    text = path.read_text()
    return set(re.findall(r'LIB_FUNCTION\("([^"]+)"', text))


def parse_elisa_nids(path: Path) -> set[str]:
    text = path.read_text()
    return set(re.findall(r'return\s+\w+\("([^"]+)"', text))


def parse_elisa_lib_nids(path: Path) -> set[str]:
    text = path.read_text()
    return set(re.findall(r'LIB_FUNCTION\("([^"]+)"', text))


def main() -> int:
    report: dict[str, dict] = {}
    for key, cpp_path in CPP_SPECS.items():
        cpp_nids = parse_cpp_nids(cpp_path)
        lib_nids = parse_elisa_lib_nids(ELISA_LIB_FILES[key])
        if key in ELISA_NID_FILES:
            table_nids = parse_elisa_nids(ELISA_NID_FILES[key])
        else:
            table_nids = lib_nids

        missing_in_elisa_lib = sorted(cpp_nids - lib_nids)
        missing_in_nid_table = sorted(cpp_nids - table_nids)
        extra_in_elisa_lib = sorted(lib_nids - cpp_nids)
        extra_in_nid_table = sorted(table_nids - cpp_nids)

        report[key] = {
            "cpp_count": len(cpp_nids),
            "elisa_lib_count": len(lib_nids),
            "elisa_table_count": len(table_nids),
            "missing_in_elisa_lib": missing_in_elisa_lib,
            "missing_in_nid_table": missing_in_nid_table,
            "extra_in_elisa_lib": extra_in_elisa_lib,
            "extra_in_nid_table": extra_in_nid_table,
            "status": "MATCHED"
            if ((key in ELISA_NID_FILES and not missing_in_nid_table) or (key not in ELISA_NID_FILES and not missing_in_elisa_lib and not missing_in_nid_table))
            else "MISMATCH",
        }

    JSON_OUT.write_text(json.dumps(report, indent=2) + "\n")

    lines = [
        "# Audio/Input Symbol + NID Coverage",
        "",
    ]
    for key in ("audioout", "audioin", "pad"):
        item = report[key]
        lines.append(f"## {key}")
        lines.append("")
        lines.append(f"- status: `{item['status']}`")
        lines.append(f"- cpp_count: `{item['cpp_count']}`")
        lines.append(f"- elisa_lib_count: `{item['elisa_lib_count']}`")
        lines.append(f"- elisa_table_count: `{item['elisa_table_count']}`")
        lines.append(f"- missing_in_elisa_lib: `{len(item['missing_in_elisa_lib'])}`")
        lines.append(f"- missing_in_nid_table: `{len(item['missing_in_nid_table'])}`")
        lines.append(f"- extra_in_elisa_lib: `{len(item['extra_in_elisa_lib'])}`")
        lines.append(f"- extra_in_nid_table: `{len(item['extra_in_nid_table'])}`")
        lines.append("")
    MD_OUT.write_text("\n".join(lines) + "\n")
    print(f"wrote {JSON_OUT.name}")
    print(f"wrote {MD_OUT.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
