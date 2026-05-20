#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CPP_FILES = [
    ROOT / "imgui/big_picture/big_picture.cpp",
    ROOT / "imgui/big_picture/settings_dialog_imgui.cpp",
    ROOT / "imgui/big_picture/settings_dialog_layer.cpp",
    ROOT / "imgui/big_picture/imgui_impl_sdl3_big_picture.cpp",
    ROOT / "imgui/big_picture/imgui_impl_sdlrenderer3.cpp",
]
ELISA_FILES = [
    ROOT / "imgui/big_picture/big_picture.elisa",
    ROOT / "imgui/big_picture/settings_dialog_imgui.elisa",
    ROOT / "imgui/big_picture/settings_dialog_layer.elisa",
    ROOT / "imgui/big_picture/imgui_impl_sdl3_big_picture.elisa",
    ROOT / "imgui/big_picture/imgui_impl_sdlrenderer3.elisa",
    ROOT / "imgui/big_picture/imgui_big_picture_ffi_bridge.elisa",
]
TEST_FILES = [
    ROOT / "imgui_big_picture_pure_tests.elisa",
    ROOT / "imgui_big_picture_ffi_bridge_tests.elisa",
]
JSON_OUT = ROOT / "imgui_big_picture_parity_matrix.json"
MD_OUT = ROOT / "imgui_big_picture_parity_matrix.md"

STATUSES = ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"]
REQUIRED_BODY_TOKENS: dict[str, list[str]] = {
    "Launch": [
        "ImguiBigPictureBridge_LaunchBegin",
        "ImGui_ImplSDL3_InitForSDLRenderer",
        "ImGui_ImplSDLRenderer3_Init",
        "ImguiBigPictureBridge_LaunchPollEventType",
        "ImguiBigPictureBridge_LaunchRenderPresent",
        "ImGui_ImplSDLRenderer3_Shutdown",
        "ImGui_ImplSDL3_Shutdown",
        "ImguiBigPictureBridge_LaunchEnd",
    ],
    "SetGameIcons": [
        "runEbootPath <-",
        "done <- true",
    ],
    "ImGui_ImplSDL3_InitForSDLRenderer": [
        "ImGui_ImplSDL3_Init",
        "IMGUI_IMPL_SDL3_BACKEND_SDLRENDERER",
    ],
    "ImGui_ImplSDLRenderer3_CreateDeviceObjects": [
        "ImguiBigPictureBridge_SDLRenderer3_CreateDeviceObjects",
        "ImGui_ImplSDLRenderer3_CreateFontsTexture",
    ],
    "SettingsWindow_DrawMainContent": [
        "closeOnSave",
        "pendingSavePopup",
        "SettingsWindow_SaveSettings",
        "SettingsWindow_DeInit",
    ],
}


@dataclass
class CppSymbol:
    display_name: str
    canonical_name: str
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


def canonical_cpp_name(name: str) -> str:
    n = name.strip()
    n = n.replace("~", "Destructor_")
    n = n.replace("::", "_")
    n = n.replace("operator()", "operator_call")
    if n.endswith("_SettingsWindow"):
        return "SettingsWindow_Create"
    if n.endswith("_Destructor_SettingsWindow"):
        return "SettingsWindow_DeInit"
    if n.endswith("_SettingsLayer"):
        return "SettingsLayer_Create"
    if n.endswith("_Destructor_SettingsLayer"):
        return "SettingsLayer_Finish"
    return n


def classify_cpp_behavior(name: str, body: str) -> str:
    squash = " ".join(line.strip() for line in body.splitlines() if line.strip())
    if "return ORBIS_OK;" in squash:
        return "STUBBED_ORBIS_OK"
    if "SDL_" in body or "ImGui_ImplSDL" in body or "stbi_" in body:
        return "HOST_BACKED"
    if "return false;" in squash or "return true;" in squash:
        return "OTHER"
    if "if (" in body and "return" in body and "nullptr" in body:
        return "ERROR_GUARDED"
    if any(tok in body for tok in ["done =", "showSettings =", "gameIcons", "currentCategory", "currentProfile", "closeOnSave", "running", "settingsLayer", "push_back", "erase("]):
        return "STATEFUL"
    return "OTHER"


def classify_elisa_behavior(body: str) -> str:
    if "ImguiBigPictureBridge_" in body:
        return "HOST_BACKED"
    if "return ORBIS_OK" in body:
        return "STUBBED_ORBIS_OK"
    if "<-" in body:
        return "STATEFUL"
    if "if " in body and "return" in body and "null" in body:
        return "ERROR_GUARDED"
    return "OTHER"


def parse_cpp_symbols(path: Path) -> list[CppSymbol]:
    text = path.read_text()
    out: list[CppSymbol] = []
    rx = re.compile(
        r"(?:^|\n)\s*(?:static\s+)?(?:[A-Za-z_][\w:<>,*&\s]*?)\s+([A-Za-z_~][\w:~]*)\s*\([^;{}]*\)\s*\{",
        re.MULTILINE,
    )
    for m in rx.finditer(text):
        raw_name = m.group(1).strip()
        start = m.end() - 1
        end = find_matching_brace(text, start)
        if end == -1:
            continue
        body = text[start + 1 : end]
        display = raw_name
        if "~" in display and "::" in display:
            cls = display.split("::", 1)[0]
            display = f"{cls}::~{cls}"
        canonical = canonical_cpp_name(display)
        out.append(
            CppSymbol(
                display_name=display,
                canonical_name=canonical,
                file=path.relative_to(ROOT).as_posix(),
                body=body,
                behavior=classify_cpp_behavior(display, body),
            )
        )
    uniq: dict[str, CppSymbol] = {}
    for s in out:
        uniq[s.canonical_name] = s
    return sorted(uniq.values(), key=lambda s: s.canonical_name)


def parse_elisa_symbols(path: Path) -> list[ElisaSymbol]:
    lines = path.read_text().splitlines()
    out: list[ElisaSymbol] = []
    for i, line in enumerate(lines):
        m = re.match(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", line)
        if not m:
            continue
        name = m.group(1)
        body_lines = []
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
                behavior=classify_elisa_behavior(body),
            )
        )
    return out


def build_coverage() -> tuple[dict[str, list[str]], set[str]]:
    per_symbol: dict[str, list[str]] = {}
    bucket_cov: set[str] = set()
    for test in TEST_FILES:
        text = test.read_text()
        rel = test.relative_to(ROOT).as_posix()
        for sym in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*?)\s*\(", text):
            per_symbol.setdefault(sym, [])
            if rel not in per_symbol[sym]:
                per_symbol[sym].append(rel)
        if "ImguiBigPictureBridge_" in text or "ImGui_ImplSDL" in text:
            bucket_cov.add("HOST_BACKED")
        if "SettingsWindow_" in text or "SettingsLayer_" in text or "BigPicture_" in text:
            bucket_cov.add("STATEFUL")
        if "== null" in text or "!= null" in text:
            bucket_cov.add("ERROR_GUARDED")
    return per_symbol, bucket_cov


def status_for(cpp: CppSymbol, elisa: ElisaSymbol | None, tests: list[str], bucket_cov: set[str]) -> str:
    if elisa is None:
        return "MISSING_IN_ELISA"

    compatible = False
    if cpp.behavior == elisa.behavior:
        compatible = True
    elif cpp.behavior == "HOST_BACKED" and elisa.behavior in {"HOST_BACKED", "STATEFUL"}:
        compatible = True
    elif cpp.behavior == "STATEFUL" and elisa.behavior in {"STATEFUL", "HOST_BACKED"}:
        compatible = True
    elif cpp.behavior == "STATEFUL" and elisa.behavior == "ERROR_GUARDED":
        compatible = True
    elif cpp.behavior == "ERROR_GUARDED" and elisa.behavior in {"ERROR_GUARDED", "STATEFUL", "HOST_BACKED"}:
        compatible = True
    elif cpp.behavior == "OTHER" and elisa.behavior in {"OTHER", "STATEFUL", "HOST_BACKED"}:
        compatible = True
    elif cpp.behavior == "STUBBED_ORBIS_OK" and elisa.behavior in {"STUBBED_ORBIS_OK", "OTHER"}:
        compatible = True

    if not compatible:
        return "MISMATCH_BEHAVIOR"

    required_tokens = REQUIRED_BODY_TOKENS.get(cpp.canonical_name, [])
    if required_tokens:
        for token in required_tokens:
            if token not in elisa.body:
                return "MISMATCH_BEHAVIOR"

    if cpp.behavior in {"HOST_BACKED", "STATEFUL", "ERROR_GUARDED"} and not tests and cpp.behavior not in bucket_cov:
        return "UNTESTED_BEHAVIOR"
    return "MATCHED"


def generate() -> dict:
    cpp_syms: dict[str, CppSymbol] = {}
    for p in CPP_FILES:
        for s in parse_cpp_symbols(p):
            cpp_syms[s.canonical_name] = s

    elisa_syms: dict[str, ElisaSymbol] = {}
    for p in ELISA_FILES:
        for s in parse_elisa_symbols(p):
            elisa_syms[s.name] = s

    coverage, bucket_cov = build_coverage()

    counts = {k: 0 for k in STATUSES}
    entries = []
    for name in sorted(cpp_syms):
        cpp = cpp_syms[name]
        elisa = elisa_syms.get(name)
        tests = coverage.get(name, [])
        status = status_for(cpp, elisa, tests, bucket_cov)
        counts[status] += 1
        entries.append(
            {
                "symbol": name,
                "cpp_symbol": cpp.display_name,
                "status": status,
                "cpp_behavior": cpp.behavior,
                "elisa_behavior": None if elisa is None else elisa.behavior,
                "cpp_file": cpp.file,
                "elisa_file": None if elisa is None else elisa.file,
                "tests": tests,
            }
        )

    return {
        "scope": "imgui/big_picture",
        "status_values": STATUSES,
        "counts": counts,
        "entries": entries,
        "behavior_bucket_coverage": sorted(bucket_cov),
        "test_files": [p.relative_to(ROOT).as_posix() for p in TEST_FILES],
    }


def write_markdown(data: dict) -> str:
    lines: list[str] = []
    lines.append("# ImGui Big Picture Parity Matrix")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    total = sum(int(v) for v in data["counts"].values())
    lines.append(f"- Total symbols: `{total}`")
    for key in STATUSES:
        lines.append(f"- {key}: `{data['counts'][key]}`")
    lines.append(f"- Behavior buckets covered by tests: `{', '.join(data.get('behavior_bucket_coverage', [])) or '-'}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | Status | C++ | Elisa | Tests |")
    lines.append("|---|---|---|---|---|")
    for e in data["entries"]:
        tests = ", ".join(e["tests"]) if e["tests"] else "-"
        lines.append(
            f"| `{e['symbol']}` | `{e['status']}` | `{e['cpp_behavior']}` | `{e['elisa_behavior'] or '-'}` | {tests} |"
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
