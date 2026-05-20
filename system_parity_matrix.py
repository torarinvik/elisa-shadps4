#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent
CPP_FILES = [
    ROOT / "core/libraries/system/commondialog.cpp",
    ROOT / "core/libraries/system/msgdialog.cpp",
    ROOT / "core/libraries/system/systemservice.cpp",
    ROOT / "core/libraries/system/userservice.cpp",
]
ELISA_FILES = [
    ROOT / "core/libraries/system/commondialog.elisa",
    ROOT / "core/libraries/system/msgdialog.elisa",
    ROOT / "core/libraries/system/systemservice.elisa",
    ROOT / "core/libraries/system/userservice.elisa",
]
TEST_FILES = [
    ROOT / "core_libraries_system_pure_tests.elisa",
    ROOT / "core_libraries_system_ffi_bridge_tests.elisa",
]
JSON_OUT = ROOT / "system_parity_matrix.json"
MD_OUT = ROOT / "system_parity_matrix.md"


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


def classify_cpp_behavior(name: str, body: str) -> str:
    body_squash = " ".join(line.strip() for line in body.splitlines() if line.strip())
    if re.fullmatch(r"return\s+ORBIS_OK\s*;", body_squash):
        return "STUBBED_ORBIS_OK"
    if "GetHostPath(" in body or "Restart(" in body:
        return "HOST_BACKED"
    if "(STUBBED) called" in body and "return ORBIS_OK;" in body:
        return "STUBBED_ORBIS_OK"
    has_error_return = "ERROR_" in body
    has_stateful = "g_" in body or "queue" in body or "push(" in body or "pop(" in body
    if has_error_return and has_stateful:
        return "STATEFUL"
    if has_error_return:
        return "ERROR_GUARDED"
    if has_stateful:
        return "STATEFUL"
    return "OTHER"


def classify_elisa_behavior(name: str, body: str) -> str:
    if "SystemService_Ffi" in body or "UserService_Ffi" in body:
        return "HOST_BACKED"
    if re.search(r"return\s+ORBIS_[A-Z0-9_]*ERROR", body):
        if "<-" in body:
            return "STATEFUL"
        return "ERROR_GUARDED"
    if "<-" in body:
        return "STATEFUL"
    if "g_" in body:
        return "STATEFUL"
    body_squash = " ".join(line.strip() for line in body.splitlines() if line.strip())
    if re.fullmatch(r"return\s+ORBIS_OK(?:\.u32\(\))?", body_squash):
        return "STUBBED_ORBIS_OK"
    return "OTHER"


def parse_cpp_symbols(path: Path) -> list[CppSymbol]:
    text = path.read_text()
    out: list[CppSymbol] = []
    for m in re.finditer(
        r'LIB_FUNCTION\([^,]+,\s*"[^"]+",\s*\d+,\s*"[^"]+",\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)',
        text,
    ):
        name = m.group(1)
        body = ""
        search_from = 0
        while True:
            idx = text.find(f"{name}(", search_from)
            if idx == -1:
                break
            brace = text.find("{", idx, idx + 600)
            semi = text.find(";", idx, idx + 600)
            if brace != -1 and (semi == -1 or brace < semi):
                end = find_matching_brace(text, brace)
                if end != -1:
                    body = text[brace + 1 : end]
                    break
            search_from = idx + len(name) + 1
        out.append(
            CppSymbol(
                name=name,
                file=path.relative_to(ROOT).as_posix(),
                body=body,
                behavior=classify_cpp_behavior(name, body),
            )
        )
    uniq: dict[str, CppSymbol] = {}
    for s in out:
        uniq[s.name] = s
    return sorted(uniq.values(), key=lambda s: s.name)


def parse_elisa_symbols(path: Path) -> list[ElisaSymbol]:
    text = path.read_text()
    lines = text.splitlines()
    out: list[ElisaSymbol] = []
    for i, line in enumerate(lines):
        m = re.match(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
        if not m:
            continue
        name = m.group(1)
        body_lines: list[str] = []
        j = i + 1
        while j < len(lines):
            nxt = lines[j]
            if nxt.startswith("    ") or nxt.strip() == "" or nxt.lstrip().startswith("#"):
                body_lines.append(nxt)
                j += 1
                continue
            break
        body = "\n".join(body_lines)
        out.append(
            ElisaSymbol(
                name=name,
                file=path.relative_to(ROOT).as_posix(),
                body=body,
                behavior=classify_elisa_behavior(name, body),
            )
        )
    return out


def build_coverage_index() -> dict[str, list[str]]:
    cov: dict[str, list[str]] = {}
    for test in TEST_FILES:
        text = test.read_text()
        for name in re.findall(r"\b(sce(?:SystemService|UserService|MsgDialog|CommonDialog)[A-Za-z0-9_]+)\b", text):
            cov.setdefault(name, [])
            rel = test.relative_to(ROOT).as_posix()
            if rel not in cov[name]:
                cov[name].append(rel)
    return cov


def status_for(cpp: CppSymbol, elisa: ElisaSymbol | None, coverage: dict[str, list[str]]) -> tuple[str, list[str]]:
    if elisa is None:
        return "MISSING_IN_ELISA", []
    tests = coverage.get(cpp.name, [])
    if cpp.behavior != elisa.behavior:
        if cpp.behavior == "STUBBED_ORBIS_OK" and elisa.behavior in {"STUBBED_ORBIS_OK", "OTHER"}:
            return "MATCHED", tests
        if cpp.behavior in {"ERROR_GUARDED", "STATEFUL"} and elisa.behavior in {"ERROR_GUARDED", "STATEFUL"}:
            if len(tests) == 0:
                return "UNTESTED_BEHAVIOR", tests
            return "MATCHED", tests
        if elisa.behavior == "HOST_BACKED" and cpp.behavior in {"ERROR_GUARDED", "STATEFUL", "HOST_BACKED"}:
            if len(tests) == 0:
                return "UNTESTED_BEHAVIOR", tests
            return "MATCHED", tests
        return "MISMATCH_BEHAVIOR", tests
    if cpp.behavior != "STUBBED_ORBIS_OK" and len(tests) == 0:
        return "UNTESTED_BEHAVIOR", tests
    return "MATCHED", tests


def generate() -> dict:
    cpp_symbols: dict[str, CppSymbol] = {}
    for path in CPP_FILES:
        for sym in parse_cpp_symbols(path):
            cpp_symbols[sym.name] = sym

    elisa_symbols: dict[str, ElisaSymbol] = {}
    for path in ELISA_FILES:
        for sym in parse_elisa_symbols(path):
            elisa_symbols[sym.name] = sym

    coverage = build_coverage_index()

    entries = []
    counts = {
        "MATCHED": 0,
        "MISMATCH_BEHAVIOR": 0,
        "MISSING_IN_ELISA": 0,
        "UNTESTED_BEHAVIOR": 0,
    }
    for name in sorted(cpp_symbols):
        cpp = cpp_symbols[name]
        elisa = elisa_symbols.get(name)
        status, tests = status_for(cpp, elisa, coverage)
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
        "scope": "core/libraries/system (commondialog, msgdialog, systemservice, userservice)",
        "status_values": ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"],
        "counts": counts,
        "entries": entries,
    }


def write_markdown(data: dict) -> str:
    lines = []
    lines.append("# System/UserService Parity Matrix")
    lines.append("")
    lines.append("## Summary")
    counts = data["counts"]
    total = sum(counts.values())
    lines.append("")
    lines.append(f"- Total symbols: `{total}`")
    for k in ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"]:
        lines.append(f"- {k}: `{counts[k]}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | Status | C++ | Elisa | Tests |")
    lines.append("|---|---|---|---|---|")
    for e in data["entries"]:
        tests = ", ".join(e["tests"]) if e["tests"] else "-"
        lines.append(
            f"| `{e['symbol']}` | `{e['status']}` | `{e['cpp_behavior']}` | "
            f"`{e['elisa_behavior'] or '-'}` | {tests} |"
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
