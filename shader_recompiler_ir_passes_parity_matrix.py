#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CPP_DIR = ROOT / "shader_recompiler/ir/passes"
ELISA_DIR = ROOT / "shader_recompiler/ir/passes"
HEADER = CPP_DIR / "ir_passes.h"
TEST_FILES = [ROOT / "shader_recompiler_ir_passes_pure_tests.elisa"]
JSON_OUT = ROOT / "shader_recompiler_ir_passes_parity_matrix.json"
MD_OUT = ROOT / "shader_recompiler_ir_passes_parity_matrix.md"

STATUSES = ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"]

EXTRA_SYMBOLS = [
    "RegisterWalkerCode",
    "SrtWalkerSignalHandler",
    "Visit",
    "CalculateSpecialSharedAtomicTypes",
    "CalculateSharedMemoryTypes",
    "ConstantPropagation",
    "Lower",
]


@dataclass
class CppSymbol:
    name: str
    file: str
    body: str
    behavior: str


@dataclass
class ElisaSymbol:
    name: str
    file: str
    body: str
    behavior: str


def find_matching_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    while i < len(text):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def parse_required_symbols() -> list[str]:
    names = set(re.findall(r"\bvoid\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", HEADER.read_text()))
    for extra in EXTRA_SYMBOLS:
        names.add(extra)
    return sorted(names)


def extract_cpp_body(name: str) -> tuple[str, str]:
    for cpp in sorted(CPP_DIR.glob("*.cpp")):
        text = cpp.read_text(errors="ignore")
        rx = re.compile(rf"\b{name}\s*\([^;{{}}]*\)\s*\{{")
        m = rx.search(text)
        if not m:
            continue
        open_idx = text.find("{", m.start())
        if open_idx == -1:
            continue
        end_idx = find_matching_brace(text, open_idx)
        if end_idx == -1:
            continue
        body = text[open_idx + 1 : end_idx]
        return cpp.relative_to(ROOT).as_posix(), body
    return "", ""


def classify_cpp_behavior(name: str, body: str) -> str:
    compact = " ".join(line.strip() for line in body.splitlines() if line.strip())
    if name in {"RegisterWalkerCode", "SrtWalkerSignalHandler"}:
        return "HOST_BACKED"
    if "return" in compact and "{" not in compact and "=" not in compact and "<-" not in compact:
        return "OTHER"
    if any(tok in body for tok in ["Xbyak", "Signals", "CodeGenerator", "DecodeInstruction"]):
        return "HOST_BACKED"
    if any(tok in body for tok in ["ASSERT", "UNREACHABLE", "LOG_WARNING"]):
        if any(mut in body for mut in ["Replace", "Set", "Invalidate", "push_back", "++", "--"]):
            return "STATEFUL"
        return "ERROR_GUARDED"
    if any(mut in body for mut in ["Replace", "Set", "Invalidate", "push_back", "++", "--", "for (", "while ("]):
        return "STATEFUL"
    return "OTHER"


def parse_elisa_symbols() -> dict[str, ElisaSymbol]:
    out: dict[str, ElisaSymbol] = {}
    for path in sorted(ELISA_DIR.glob("*.elisa")):
        lines = path.read_text().splitlines()
        for i, line in enumerate(lines):
            m = re.match(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
            if not m:
                continue
            name = m.group(1)
            j = i + 1
            body_lines: list[str] = []
            while j < len(lines):
                nxt = lines[j]
                if nxt.startswith("    ") or nxt.strip() == "" or nxt.lstrip().startswith("#"):
                    body_lines.append(nxt)
                    j += 1
                    continue
                break
            body = "\n".join(body_lines)
            behavior = classify_elisa_behavior(name, body)
            out[name] = ElisaSymbol(
                name=name,
                file=path.relative_to(ROOT).as_posix(),
                body=body,
                behavior=behavior,
            )
    return out


def classify_elisa_behavior(name: str, body: str) -> str:
    if name in {"RegisterWalkerCode", "SrtWalkerSignalHandler"}:
        return "HOST_BACKED"
    if any(tok in body for tok in ["<-", "ShaderIrPass_", "AddInst", "RemoveInst", "ReplaceWithImm", "ReplaceOpcode"]):
        return "STATEFUL"
    if "if " in body and "return" in body:
        return "ERROR_GUARDED"
    return "OTHER"


def build_coverage() -> dict[str, list[str]]:
    out: dict[str, list[str]] = {}
    for test in TEST_FILES:
        text = test.read_text()
        rel = test.relative_to(ROOT).as_posix()
        for name in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(", text):
            out.setdefault(name, [])
            if rel not in out[name]:
                out[name].append(rel)
    return out


def status_for(cpp: CppSymbol, elisa: ElisaSymbol | None, tests: list[str]) -> str:
    if elisa is None:
        return "MISSING_IN_ELISA"

    compatible = cpp.behavior == elisa.behavior
    if not compatible and cpp.behavior == "STATEFUL" and elisa.behavior in {"STATEFUL", "ERROR_GUARDED"}:
        compatible = True
    if not compatible and cpp.behavior == "ERROR_GUARDED" and elisa.behavior in {"ERROR_GUARDED", "STATEFUL"}:
        compatible = True
    if not compatible and cpp.behavior == "OTHER" and elisa.behavior in {"OTHER", "ERROR_GUARDED"}:
        compatible = True

    if not compatible:
        return "MISMATCH_BEHAVIOR"

    if cpp.behavior in {"STATEFUL", "HOST_BACKED", "ERROR_GUARDED"} and len(tests) == 0:
        return "UNTESTED_BEHAVIOR"
    return "MATCHED"


def generate() -> dict:
    required = parse_required_symbols()
    elisa_symbols = parse_elisa_symbols()
    coverage = build_coverage()

    entries: list[dict] = []
    counts = {k: 0 for k in STATUSES}

    for name in required:
        cpp_file, body = extract_cpp_body(name)
        cpp = CppSymbol(
            name=name,
            file=cpp_file,
            body=body,
            behavior=classify_cpp_behavior(name, body),
        )
        elisa = elisa_symbols.get(name)
        tests = coverage.get(name, [])
        status = status_for(cpp, elisa, tests)
        counts[status] += 1
        entries.append(
            {
                "symbol": name,
                "status": status,
                "cpp_behavior": cpp.behavior,
                "elisa_behavior": None if elisa is None else elisa.behavior,
                "cpp_file": cpp.file,
                "elisa_file": None if elisa is None else elisa.file,
                "tests": tests,
            }
        )

    return {
        "scope": "shader_recompiler/ir/passes",
        "status_values": STATUSES,
        "counts": counts,
        "entries": entries,
        "required_symbols": required,
        "test_files": [p.relative_to(ROOT).as_posix() for p in TEST_FILES],
    }


def write_markdown(data: dict) -> str:
    lines: list[str] = []
    lines.append("# Shader Recompiler IR Passes Parity Matrix")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    total = sum(int(v) for v in data["counts"].values())
    lines.append(f"- Total symbols: `{total}`")
    for key in STATUSES:
        lines.append(f"- {key}: `{data['counts'][key]}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | Status | C++ | Elisa | Tests |")
    lines.append("|---|---|---|---|---|")
    for entry in data["entries"]:
        tests = ", ".join(entry["tests"]) if entry["tests"] else "-"
        lines.append(
            f"| `{entry['symbol']}` | `{entry['status']}` | `{entry['cpp_behavior']}` | "
            f"`{entry['elisa_behavior'] or '-'}` | {tests} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    data = generate()
    JSON_OUT.write_text(json.dumps(data, indent=2) + "\n")
    MD_OUT.write_text(write_markdown(data))
    print(f"wrote {JSON_OUT.relative_to(ROOT)}")
    print(f"wrote {MD_OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
