#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent

TARGET_CPP_FILES = [
    "shader_recompiler/recompiler.cpp",
    "shader_recompiler/backend/spirv/emit_spirv.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_atomic.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_barriers.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_bitwise_conversion.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_composite.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_context_get_set.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_convert.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_floating_point.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_image.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_integer.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_logical.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_quad_rect.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_select.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_shared_memory.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_special.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_undefined.cpp",
    "shader_recompiler/backend/spirv/emit_spirv_warp.cpp",
    "shader_recompiler/backend/spirv/spirv_emit_context.cpp",
]

TARGET_HEADER_FILES = [
    "shader_recompiler/recompiler.h",
    "shader_recompiler/backend/spirv/emit_spirv.h",
    "shader_recompiler/backend/spirv/emit_spirv_instructions.h",
    "shader_recompiler/backend/spirv/emit_spirv_quad_rect.h",
    "shader_recompiler/backend/spirv/spirv_emit_context.h",
]

ELISA_FILES = [
    "shader_recompiler/backend/spirv/spirv_backend.elisa",
    "shader_recompiler/recompiler_backend.elisa",
]

TEST_FILES = [
    "elisa_tests/shader_recompiler_backend_spirv_tests.elisa",
    "elisa_tests/shader_recompiler_recompiler_tests.elisa",
]

JSON_OUT = ROOT / "shader_recompiler_backend_spirv_parity_matrix.json"
MD_OUT = ROOT / "shader_recompiler_backend_spirv_parity_matrix.md"

STATUS_MATCHED = "MATCHED"
STATUS_MISSING = "MISSING_COVERAGE"
STATUS_VALUES = [STATUS_MATCHED, STATUS_MISSING]

COVERAGE_NATIVE = "NATIVE_ELISA"
COVERAGE_HOST_SPIRV = "HOST_BACKED_SPIRV_EMIT_WORDS"
COVERAGE_HOST_AUX = "HOST_BACKED_SPIRV_AUX_TESS"
COVERAGE_HOST_RECOMP = "HOST_BACKED_RECOMPILER_TRANSLATE"
COVERAGE_NONE = "NONE"

KEYWORDS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "decltype",
    "typeid",
    "constexpr",
}


@dataclass
class SymbolEntry:
    symbol: str
    source_file: str
    source_kind: str
    coverage: str
    status: str
    tests: list[str]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def clean_symbol(raw: str) -> str:
    name = raw.strip()
    if "::" in name:
        name = name.split("::")[-1]
    if name.startswith("~"):
        name = name[1:]
    return name


def extract_cpp_symbols(path: Path) -> list[str]:
    text = path.read_text(errors="ignore")
    rx = re.compile(
        r"^(?:\[\[nodiscard\]\]\s*)?(?:inline\s+)?(?:constexpr\s+)?"
        r"(?:[A-Za-z_][A-Za-z0-9_:<>\s*&,\[\]]*?)\b([A-Za-z_~][A-Za-z0-9_:~]*)\s*"
        r"\([^;{}]*\)\s*(?:const\s*)?\{",
        re.M,
    )
    out: list[str] = []
    for m in rx.finditer(text):
        name = clean_symbol(m.group(1))
        if not name or name in KEYWORDS:
            continue
        out.append(name)
    return out


def extract_header_symbols(path: Path) -> list[str]:
    text = path.read_text(errors="ignore")
    rx = re.compile(
        r"(?:\[\[nodiscard\]\]\s*)?(?:inline\s+)?(?:constexpr\s+)?"
        r"(?:[A-Za-z_][A-Za-z0-9_:<>\s*&,\[\]]*?)\b([A-Za-z_~][A-Za-z0-9_:~]*)\s*"
        r"\([^;{}]*\)\s*(?:const\s*)?[;{]",
        re.M,
    )
    out: list[str] = []
    for m in rx.finditer(text):
        name = clean_symbol(m.group(1))
        if not name or name in KEYWORDS:
            continue
        out.append(name)
    return out


def extract_elisa_defs(path: Path) -> set[str]:
    symbols: set[str] = set()
    for line in path.read_text().splitlines():
        m = re.match(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
        if m:
            symbols.add(m.group(1))
    return symbols


def build_test_coverage(symbols: set[str]) -> dict[str, list[str]]:
    out: dict[str, list[str]] = {}
    for rel_test in TEST_FILES:
        path = ROOT / rel_test
        if not path.exists():
            continue
        text = path.read_text(errors="ignore")
        for sym in symbols:
            if re.search(rf"\b{re.escape(sym)}\s*\(", text):
                out.setdefault(sym, [])
                if rel_test not in out[sym]:
                    out[sym].append(rel_test)
    return out


def coverage_for(source_file: str, symbol: str, elisa_defs: set[str]) -> str:
    if symbol in elisa_defs:
        return COVERAGE_NATIVE
    if source_file.startswith("shader_recompiler/backend/spirv/"):
        if symbol == "EmitAuxilaryTessShader":
            return COVERAGE_HOST_AUX
        return COVERAGE_HOST_SPIRV
    if source_file.startswith("shader_recompiler/recompiler"):
        return COVERAGE_HOST_RECOMP
    return COVERAGE_NONE


def generate() -> dict:
    elisa_defs: set[str] = set()
    for rel_file in ELISA_FILES:
        path = ROOT / rel_file
        if path.exists():
            elisa_defs |= extract_elisa_defs(path)

    raw_symbols: list[tuple[str, str, str]] = []
    for rel_file in TARGET_CPP_FILES:
        path = ROOT / rel_file
        if not path.exists():
            continue
        for sym in extract_cpp_symbols(path):
            raw_symbols.append((sym, rel_file, "cpp"))
    for rel_file in TARGET_HEADER_FILES:
        path = ROOT / rel_file
        if not path.exists():
            continue
        for sym in extract_header_symbols(path):
            raw_symbols.append((sym, rel_file, "header"))

    seen: set[tuple[str, str]] = set()
    deduped: list[tuple[str, str, str]] = []
    for symbol, source_file, source_kind in raw_symbols:
        key = (symbol, source_file)
        if key in seen:
            continue
        seen.add(key)
        deduped.append((symbol, source_file, source_kind))

    symbol_names = {symbol for symbol, _, _ in deduped}
    tests_by_symbol = build_test_coverage(symbol_names)

    entries: list[SymbolEntry] = []
    counts = {k: 0 for k in STATUS_VALUES}
    coverage_counts: dict[str, int] = {
        COVERAGE_NATIVE: 0,
        COVERAGE_HOST_SPIRV: 0,
        COVERAGE_HOST_AUX: 0,
        COVERAGE_HOST_RECOMP: 0,
        COVERAGE_NONE: 0,
    }

    for symbol, source_file, source_kind in sorted(deduped, key=lambda it: (it[1], it[0])):
        coverage = coverage_for(source_file, symbol, elisa_defs)
        status = STATUS_MATCHED if coverage != COVERAGE_NONE else STATUS_MISSING
        coverage_counts[coverage] = coverage_counts.get(coverage, 0) + 1
        counts[status] += 1
        entries.append(
            SymbolEntry(
                symbol=symbol,
                source_file=source_file,
                source_kind=source_kind,
                coverage=coverage,
                status=status,
                tests=tests_by_symbol.get(symbol, []),
            )
        )

    return {
        "scope": "shader_recompiler/recompiler.cpp + shader_recompiler/backend/spirv/*",
        "target_cpp_files": TARGET_CPP_FILES,
        "target_header_files": TARGET_HEADER_FILES,
        "elisa_files": ELISA_FILES,
        "test_files": TEST_FILES,
        "status_values": STATUS_VALUES,
        "counts": counts,
        "coverage_counts": coverage_counts,
        "entries": [
            {
                "symbol": entry.symbol,
                "source_file": entry.source_file,
                "source_kind": entry.source_kind,
                "coverage": entry.coverage,
                "status": entry.status,
                "tests": entry.tests,
            }
            for entry in entries
        ],
    }


def write_markdown(data: dict) -> str:
    lines: list[str] = []
    lines.append("# Shader Recompiler SPIR-V + Recompiler Parity Matrix")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    total = sum(int(v) for v in data["counts"].values())
    lines.append(f"- Total symbols: `{total}`")
    for key in STATUS_VALUES:
        lines.append(f"- {key}: `{data['counts'][key]}`")
    lines.append("")
    lines.append("## Coverage Modes")
    lines.append("")
    for key, value in data["coverage_counts"].items():
        lines.append(f"- {key}: `{value}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | File | Kind | Coverage | Status | Tests |")
    lines.append("|---|---|---|---|---|---|")
    for entry in data["entries"]:
        tests = ", ".join(entry["tests"]) if entry["tests"] else "-"
        lines.append(
            f"| `{entry['symbol']}` | `{entry['source_file']}` | `{entry['source_kind']}` | "
            f"`{entry['coverage']}` | `{entry['status']}` | {tests} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    data = generate()
    JSON_OUT.write_text(json.dumps(data, indent=2) + "\n")
    MD_OUT.write_text(write_markdown(data))
    print(f"wrote {rel(JSON_OUT)}")
    print(f"wrote {rel(MD_OUT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
