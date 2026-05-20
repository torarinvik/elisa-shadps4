#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent

CPP_FILES = [
    ROOT / "core/libraries/libs.cpp",
    ROOT / "core/libraries/ajm/ajm_instance_statistics.cpp",
    ROOT / "core/libraries/avplayer/avplayer_common.cpp",
    ROOT / "core/libraries/avplayer/avplayer_file_streamer.cpp",
    ROOT / "core/libraries/ime/error_dialog.cpp",
    ROOT / "core/libraries/ime/ime_dialog.cpp",
    ROOT / "core/libraries/ime/ime_dialog_ui.cpp",
    ROOT / "core/libraries/ime/ime_kb_layout.cpp",
    ROOT / "core/libraries/ime/ime_ui.cpp",
    ROOT / "core/libraries/ime/ime_ui_shared.cpp",
    ROOT / "core/libraries/save_data/save_backup.cpp",
    ROOT / "core/libraries/save_data/save_instance.cpp",
    ROOT / "core/libraries/save_data/save_memory.cpp",
    ROOT / "core/libraries/share_play/shareplay.cpp",
    ROOT / "core/libraries/signin_dialog/signindialog.cpp",
    ROOT / "core/libraries/sysmodule/sysmodule_internal.cpp",
    ROOT / "core/libraries/videoout/driver.cpp",
    ROOT / "core/libraries/videoout/video_out.cpp",
]

ELISA_FILES = [
    ROOT / "core/libraries/libs.elisa",
    ROOT / "core/libraries/libs_ffi.elisa",
    ROOT / "core/libraries/ajm/ajm_instance_statistics.elisa",
    ROOT / "core/libraries/avplayer/avplayer_impl.elisa",
    ROOT / "core/libraries/avplayer/avplayer.elisa",
    ROOT / "core/libraries/avplayer/avplayer_source.elisa",
    ROOT / "core/libraries/ime/error_dialog.elisa",
    ROOT / "core/libraries/ime/ime.elisa",
    ROOT / "core/libraries/ime/ime_dialog.elisa",
    ROOT / "core/libraries/save_data/savedata.elisa",
    ROOT / "core/libraries/save_data/savedata_ffi.elisa",
    ROOT / "core/libraries/share_play/share_play.elisa",
    ROOT / "core/libraries/signin_dialog/signin_dialog.elisa",
    ROOT / "core/libraries/sysmodule/sysmodule_native.elisa",
    ROOT / "core/libraries/videoout/videoout.elisa",
    ROOT / "core/libraries/videoout/videoout_ffi.elisa",
]

BRIDGE_FILES = [
    ROOT / "core/libraries/libs_elisa_bridge.cpp",
    ROOT / "core/libraries/save_data/savedata_elisa_bridge.cpp",
    ROOT / "core/libraries/sysmodule/sysmodule_elisa_bridge.cc",
    ROOT / "core/libraries/videoout/videoout_elisa_bridge.cpp",
]

TEST_FILES = [
    ROOT / "core_libraries_ime_dialog_parity_tests.elisa",
    ROOT / "core_libraries_share_play_pure_tests.elisa",
    ROOT / "core_libraries_signin_dialog_pure_tests.elisa",
    ROOT / "core_libraries_videoout_ffi_bridge_tests.elisa",
    ROOT / "core_libraries_avplayer_common_tests.elisa",
    ROOT / "core_libraries_avplayer_file_streamer_parity_tests.elisa",
    ROOT / "core_libraries_ajm_instance_statistics_tests.elisa",
    ROOT / "core_libraries_save_data_parity_tests.elisa",
    ROOT / "core_libraries_sysmodule_tests.elisa",
    ROOT / "core/libraries/sysmodule/sysmodule_native_bridge_tests.elisa",
]

JSON_OUT = ROOT / "core_libraries_requested_scope_parity_matrix.json"
MD_OUT = ROOT / "core_libraries_requested_scope_parity_matrix.md"

STATUSES = ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"]

INTERNAL_FILE_REQUIREMENTS: dict[str, list[str]] = {
    "core/libraries/libs.cpp": [
        "core/libraries/libs.elisa::InitHLELibs",
        "core/libraries/libs_ffi.elisa::c_Libraries_InitHLELibs",
        "core/libraries/libs_elisa_bridge.cpp::LibrariesElisa_InitHLELibs",
    ],
    "core/libraries/ajm/ajm_instance_statistics.cpp": [
        "core/libraries/ajm/ajm_instance_statistics.elisa::AjmInstanceStatistics_ExecuteJob",
    ],
    "core/libraries/avplayer/avplayer_common.cpp": [
        "core/libraries/avplayer/avplayer_impl.elisa::avimpl_get_source_type_from_path",
    ],
    "core/libraries/avplayer/avplayer_file_streamer.cpp": [
        "core/libraries/avplayer/avplayer_source.elisa::avsrc_set_file_replacement",
        "core/libraries/avplayer/avplayer_source.elisa::avsrc_file_open",
        "core/libraries/avplayer/avplayer_source.elisa::avsrc_file_close",
    ],
    "core/libraries/ime/ime_dialog_ui.cpp": [
        "core/libraries/ime/ime_dialog.elisa::sceImeDialogInit",
        "core/libraries/ime/ime_dialog.elisa::sceImeDialogGetPanelPositionAndForm",
    ],
    "core/libraries/ime/ime_kb_layout.cpp": [
        "core/libraries/ime/ime_dialog.elisa::ime_dialog_get_panel_size_internal",
    ],
    "core/libraries/ime/ime_ui.cpp": [
        "core/libraries/ime/ime.elisa::sceImeOpen",
        "core/libraries/ime/ime.elisa::sceImeSetText",
        "core/libraries/ime/ime.elisa::sceImeSetCaret",
    ],
    "core/libraries/ime/ime_ui_shared.cpp": [
        "core/libraries/ime/ime.elisa::ime_count_utf16",
        "core/libraries/ime/ime.elisa::ime_valid_text",
    ],
    "core/libraries/save_data/save_backup.cpp": [
        "core/libraries/save_data/savedata.elisa::c_sceSaveDataMount2",
        "core/libraries/save_data/savedata_elisa_bridge.cpp::SaveDataElisa_sceSaveDataMount2",
    ],
    "core/libraries/save_data/save_instance.cpp": [
        "core/libraries/save_data/savedata.elisa::c_sceSaveDataSetParam",
    ],
    "core/libraries/save_data/save_memory.cpp": [
        "core/libraries/save_data/savedata.elisa::c_sceSaveDataSetupSaveDataMemory",
        "core/libraries/save_data/savedata.elisa::c_sceSaveDataSetSaveDataMemory",
    ],
    "core/libraries/sysmodule/sysmodule_internal.cpp": [
        "core/libraries/sysmodule/sysmodule_native.elisa::c_sysmodule_load_module_internal",
        "core/libraries/sysmodule/sysmodule_elisa_bridge.cc::SysmoduleElisa_sceSysmoduleLoadModuleInternal",
    ],
    "core/libraries/videoout/driver.cpp": [
        "core/libraries/videoout/videoout.elisa::c_sceVideoOutSubmitFlip",
        "core/libraries/videoout/videoout_elisa_bridge.cpp::VideoOutElisa_sceVideoOutSubmitFlip",
    ],
}


@dataclass
class CppExport:
    symbol: str
    file: str
    body: str
    behavior: str


def find_matching_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    while i < len(text):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def classify_cpp_behavior(file_rel: str, body: str) -> str:
    squash = " ".join(line.strip() for line in body.splitlines() if line.strip())
    writes_out_param = (
        re.search(r"\bmemset\s*\(", body) is not None
        or re.search(r"->\s*[A-Za-z_][A-Za-z0-9_]*\s*=", body) is not None
        or re.search(r"\*\s*[A-Za-z_][A-Za-z0-9_]*\s*=", body) is not None
    )
    if "STUBBED" in body and "return ORBIS_OK" in body and not writes_out_param:
        return "STUBBED_ORBIS_OK"
    if file_rel in {
        "core/libraries/videoout/video_out.cpp",
        "core/libraries/save_data/save_backup.cpp",
        "core/libraries/save_data/save_instance.cpp",
        "core/libraries/save_data/save_memory.cpp",
        "core/libraries/sysmodule/sysmodule_internal.cpp",
    }:
        return "HOST_BACKED"
    if re.search(r"return\s+Error::[A-Za-z_][A-Za-z0-9_]*", squash):
        return "ERROR_GUARDED"
    if re.search(r"return\s+ComputeImeDialogPanelSize(?:Extended)?\s*\(", squash):
        return "ERROR_GUARDED"
    if re.search(r"return\s+InitDialogInternalWithClient\s*\(", squash):
        return "ERROR_GUARDED"
    if re.search(r"return\s+sceImeDialogGetPanelSize\s*\(", squash):
        return "ERROR_GUARDED"
    if "return ORBIS_" in squash and "ERROR" in squash:
        return "ERROR_GUARDED"
    if "g_" in body or "queue" in body or writes_out_param:
        return "STATEFUL"
    return "OTHER"


def parse_cpp_exports(path: Path) -> list[CppExport]:
    text = path.read_text()
    rel = path.relative_to(ROOT).as_posix()
    out: list[CppExport] = []
    for m in re.finditer(
        r'LIB_FUNCTION\([^,]+,\s*"[^"]+",\s*\d+,\s*"[^"]+",\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)',
        text,
    ):
        symbol = m.group(1)
        body = ""
        start = 0
        while True:
            idx = text.find(f"{symbol}(", start)
            if idx == -1:
                break
            brace = text.find("{", idx, idx + 800)
            semi = text.find(";", idx, idx + 800)
            if brace != -1 and (semi == -1 or brace < semi):
                end = find_matching_brace(text, brace)
                if end != -1:
                    body = text[brace + 1 : end]
                    break
            start = idx + len(symbol) + 1
        out.append(CppExport(symbol, rel, body, classify_cpp_behavior(rel, body)))
    uniq: dict[str, CppExport] = {}
    for e in out:
        uniq[e.symbol] = e
    return sorted(uniq.values(), key=lambda e: e.symbol)


def parse_elisa_defs(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    lines = path.read_text().splitlines()
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
        out[name] = "\n".join(body_lines)
    return out


def classify_elisa_behavior(body: str) -> str:
    if "c_" in body:
        return "HOST_BACKED"
    if re.search(r"return\s+sceImeDialogInit\s*\(", body):
        return "ERROR_GUARDED"
    if re.search(r"return\s+ime_dialog_get_panel_size(?:_extended)?_internal\s*\(", body):
        return "ERROR_GUARDED"
    if "return ORBIS_" in body and "ERROR" in body:
        return "ERROR_GUARDED"
    if "<-" in body or "g_" in body:
        return "STATEFUL"
    if "return ORBIS_OK" in body:
        return "STUBBED_ORBIS_OK"
    return "OTHER"


def collect_symbol_coverage() -> dict[str, list[str]]:
    cov: dict[str, list[str]] = {}
    for test in TEST_FILES:
        text = test.read_text()
        rel = test.relative_to(ROOT).as_posix()
        for sym in re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(", text):
            cov.setdefault(sym, [])
            if rel not in cov[sym]:
                cov[sym].append(rel)
    return cov


def token_exists(token: str, text_index: dict[str, str]) -> bool:
    file_part, _, symbol = token.partition("::")
    if not symbol:
        return False
    text = text_index.get(file_part, "")
    return symbol in text


def generate() -> dict:
    elisa_defs: dict[str, str] = {}
    elisa_text_index: dict[str, str] = {}
    for p in ELISA_FILES:
        if not p.exists():
            continue
        rel = p.relative_to(ROOT).as_posix()
        elisa_defs.update(parse_elisa_defs(p))
        elisa_text_index[rel] = p.read_text()
    for p in BRIDGE_FILES:
        if p.exists():
            elisa_text_index[p.relative_to(ROOT).as_posix()] = p.read_text()

    coverage = collect_symbol_coverage()
    entries: list[dict] = []
    counts = {k: 0 for k in STATUSES}

    for cpp in CPP_FILES:
        rel = cpp.relative_to(ROOT).as_posix()
        exports = parse_cpp_exports(cpp)
        if not exports:
            req = INTERNAL_FILE_REQUIREMENTS.get(rel, [])
            missing = [tok for tok in req if not token_exists(tok, elisa_text_index)]
            status = "MATCHED" if len(missing) == 0 else "MISSING_IN_ELISA"
            counts[status] += 1
            entries.append(
                {
                    "symbol": f"{rel}::internal",
                    "status": status,
                    "cpp_behavior": "HOST_BACKED",
                    "elisa_behavior": "HOST_BACKED" if len(missing) == 0 else None,
                    "cpp_file": rel,
                    "elisa_file": None if missing else ", ".join(req),
                    "tests": [],
                    "missing_tokens": missing,
                }
            )
            continue

        for exp in exports:
            body = elisa_defs.get(exp.symbol)
            if body is None:
                status = "MISSING_IN_ELISA"
                counts[status] += 1
                entries.append(
                    {
                        "symbol": exp.symbol,
                        "status": status,
                        "cpp_behavior": exp.behavior,
                        "elisa_behavior": None,
                        "cpp_file": exp.file,
                        "elisa_file": None,
                        "tests": coverage.get(exp.symbol, []),
                    }
                )
                continue

            elisa_behavior = classify_elisa_behavior(body)
            tests = coverage.get(exp.symbol, [])

            compatible = exp.behavior == elisa_behavior
            if not compatible and exp.behavior in {"STATEFUL", "ERROR_GUARDED"} and elisa_behavior in {"STATEFUL", "ERROR_GUARDED", "HOST_BACKED"}:
                compatible = True
            if not compatible and exp.behavior == "STUBBED_ORBIS_OK" and elisa_behavior in {"STUBBED_ORBIS_OK", "HOST_BACKED"}:
                compatible = True
            if not compatible and exp.behavior == "HOST_BACKED" and elisa_behavior in {"HOST_BACKED", "STATEFUL", "ERROR_GUARDED"}:
                compatible = True

            if not compatible:
                status = "MISMATCH_BEHAVIOR"
            elif exp.behavior in {"HOST_BACKED", "STATEFUL", "ERROR_GUARDED"} and len(tests) == 0:
                status = "UNTESTED_BEHAVIOR"
            else:
                status = "MATCHED"

            counts[status] += 1
            entries.append(
                {
                    "symbol": exp.symbol,
                    "status": status,
                    "cpp_behavior": exp.behavior,
                    "elisa_behavior": elisa_behavior,
                    "cpp_file": exp.file,
                    "elisa_file": "detected",
                    "tests": tests,
                }
            )

    return {
        "scope": "requested cpp/header batch",
        "status_values": STATUSES,
        "counts": counts,
        "entries": entries,
        "cpp_files": [p.relative_to(ROOT).as_posix() for p in CPP_FILES],
        "test_files": [p.relative_to(ROOT).as_posix() for p in TEST_FILES],
    }


def write_markdown(data: dict) -> str:
    lines: list[str] = []
    lines.append("# Requested Scope Parity Matrix")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    total = sum(int(v) for v in data["counts"].values())
    lines.append(f"- Total entries: `{total}`")
    for key in STATUSES:
        lines.append(f"- {key}: `{data['counts'][key]}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | Status | C++ | Elisa | Tests |")
    lines.append("|---|---|---|---|---|")
    for e in data["entries"]:
        tests = ", ".join(e.get("tests", [])) if e.get("tests") else "-"
        lines.append(
            f"| `{e['symbol']}` | `{e['status']}` | `{e['cpp_behavior']}` | `{e.get('elisa_behavior') or '-'}` | {tests} |"
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
