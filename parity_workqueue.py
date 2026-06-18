#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
UPSTREAM = ROOT.parent / "shadPS4" / "src"
EXCLUDE_PARTS = {
    ".git",
    "externals",
    "build",
    "cmake-build-debug",
    "third_party",
    "tests",
}
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh"}
OVERRIDES: dict[str, tuple[str, str]] = {
    "imgui/imgui_config": (
        "Temporary-Cpp-Bridge",
        "Dear ImGui compile-time configuration for host UI bridge; retire when native Elisa ImGui renderer owns this layer",
    ),
    "imgui/imgui_layer": (
        "Temporary-Cpp-Bridge",
        "host UI layer abstraction covered by current ImGui bridge surface",
    ),
    "imgui/imgui_std": (
        "Temporary-Cpp-Bridge",
        "Dear ImGui host helper surface covered by current ImGui bridge surface",
    ),
    "imgui/imgui_texture": (
        "Temporary-Cpp-Bridge",
        "host UI texture lifetime helper covered by current ImGui bridge surface",
    ),
    "imgui/imgui_translations": (
        "Temporary-Cpp-Bridge",
        "host UI translation table remains with ImGui bridge until native settings UI is ported",
    ),
    "imgui/notifications_layer": (
        "Temporary-Cpp-Bridge",
        "host notification UI layer remains with ImGui bridge until native overlay UI is ported",
    ),
    "video_core/host_shaders/fsr/ffx_a": (
        "Not-Applicable-External",
        "AMD FidelityFX shader portability header consumed as shader source data",
    ),
    "video_core/host_shaders/fsr/ffx_fsr1": (
        "Not-Applicable-External",
        "AMD FidelityFX FSR shader header consumed as shader source data",
    ),
    "video_core/host_diagnostics": (
        "Temporary-Cpp-Bridge",
        "host-only GPU command-recording validation/diagnostics (pure log/assert, never alters rendering); stays as C++ developer tooling, not part of the emulation surface to port",
    ),
    "video_core/renderer_vulkan/vk_gpu_command_diagnostics": (
        "Missing",
        "Milestone C renderer diagnostic runtime helper; port native Elisa ring/env behavior or bridge behind Vulkan host API",
    ),
    "video_core/renderer_vulkan/vk_wait_diagnostics": (
        "Missing",
        "Milestone C renderer wait-timeout helper; port native Elisa env parsing and retry/fatal policy",
    ),
}


def rel(path: Path, base: Path) -> str:
    try:
        return path.relative_to(base).as_posix()
    except ValueError:
        return path.as_posix()


def is_project_owned(path: Path) -> bool:
    parts = set(path.parts)
    if parts & EXCLUDE_PARTS:
        return False
    name = path.name.lower()
    if name.endswith(".generated.cpp") or name.endswith(".generated.h"):
        return False
    return path.suffix in SOURCE_SUFFIXES


def collect_cpp_units() -> list[tuple[str, Path]]:
    roots: list[tuple[str, Path]] = [("elisa", ROOT)]
    if UPSTREAM.exists():
        roots.append(("cpp", UPSTREAM))
    units: dict[str, tuple[str, Path]] = {}
    for label, root in roots:
        for path in root.rglob("*"):
            if not path.is_file() or not is_project_owned(path):
                continue
            if path.suffix in {".c"}:
                continue
            key = path.with_suffix("").relative_to(root).as_posix()
            units.setdefault(key, (label, path))
    return [(k, v[1]) for k, v in sorted(units.items())]


def collect_elisa_stems() -> set[str]:
    stems: set[str] = set()
    for path in ROOT.rglob("*.elisa"):
        if not is_project_path(path):
            continue
        stems.add(path.with_suffix("").relative_to(ROOT).as_posix())
    return stems


def is_project_path(path: Path) -> bool:
    return not (set(path.parts) & EXCLUDE_PARTS)


def bridge_signals() -> set[str]:
    signals: set[str] = set()
    for path in ROOT.rglob("*"):
        if not path.is_file() or not is_project_path(path):
            continue
        low = path.name.lower()
        if "bridge" in low or "shim" in low:
            signals.add(path.with_suffix("").relative_to(ROOT).as_posix())
            signals.add(path.parent.relative_to(ROOT).as_posix())
    return signals


def classify(key: str, path: Path, elisa: set[str], bridges: set[str]) -> tuple[str, str]:
    if key in elisa:
        return "Native-Elisa", "matching Elisa source stem exists"
    if key in OVERRIDES:
        return OVERRIDES[key]
    parent = str(Path(key).parent)
    if any(sig == key or sig == parent or key.startswith(sig + "/") for sig in bridges):
        return "Temporary-Cpp-Bridge", "nearby bridge/shim file exists; confirm blocker and retirement plan in parity_ledger.json"
    if "/externals/" in path.as_posix() or "externals" in path.parts:
        return "Not-Applicable-External", "external dependency"
    return "Missing", "no matching Elisa stem or bridge signal found"


def build_rows(limit: int | None = None) -> list[dict[str, str]]:
    elisa = collect_elisa_stems()
    bridges = bridge_signals()
    rows: list[dict[str, str]] = []
    for key, path in collect_cpp_units():
        status, reason = classify(key, path, elisa, bridges)
        rows.append({
            "unit": key,
            "source": rel(path, ROOT),
            "status": status,
            "reason": reason,
        })
    if limit is not None:
        return rows[:limit]
    return rows


def print_summary(rows: list[dict[str, str]]) -> None:
    counts: dict[str, int] = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    print(f"project-owned c++ units scanned: {len(rows)}")
    for key in sorted(counts):
        print(f"{key}: {counts[key]}")


def write_markdown(path: Path, rows: list[dict[str, str]]) -> None:
    lines = ["# Parity Workqueue", "", "Generated from project-owned C++ source/header units.", ""]
    counts: dict[str, int] = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
    lines.append("## Summary")
    for key in sorted(counts):
        lines.append(f"- {key}: {counts[key]}")
    lines.extend(["", "## Units", "", "| Status | Unit | Source | Reason |", "|---|---|---|---|"])
    for row in rows:
        lines.append(f"| {row['status']} | `{row['unit']}` | `{row['source']}` | {row['reason']} |")
    path.write_text("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a C++ -> Elisa parity workqueue from source units.")
    parser.add_argument("--json", type=Path, help="Write JSON workqueue to this path.")
    parser.add_argument("--markdown", type=Path, help="Write Markdown workqueue to this path.")
    parser.add_argument("--fail-missing", action="store_true", help="Fail if any unit is Missing.")
    args = parser.parse_args()

    rows = build_rows()
    print_summary(rows)
    if args.json:
        args.json.write_text(json.dumps({"units": rows}, indent=2) + "\n")
        print(f"wrote json: {args.json}")
    if args.markdown:
        write_markdown(args.markdown, rows)
        print(f"wrote markdown: {args.markdown}")
    missing = sum(1 for row in rows if row["status"] == "Missing")
    return 1 if args.fail_missing and missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
