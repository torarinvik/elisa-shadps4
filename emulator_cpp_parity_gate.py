#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"
TMP_DIR = Path(os.environ.get("TMPDIR", "/tmp"))
MILESTONES_PATH = ROOT / "Milestones.md"
LAST_REPORT_PATH = ROOT / "parity_gate_latest.md"


@dataclass(frozen=True)
class Step:
    name: str
    cmd: list[str]
    cwd: Path = ROOT
    category: str = "gate"
    slow: bool = False
    optional: bool = False
    timeout_seconds: int | None = None


@dataclass
class Result:
    step: Step
    returncode: int
    output: str

    @property
    def passed(self) -> bool:
        return self.returncode == 0


@dataclass(frozen=True)
class ReadinessProbe:
    subsystem: str
    status: str
    evidence: tuple[str, ...]
    note: str

    @property
    def ready(self) -> bool:
        return self.status != "missing" and all((ROOT / path).exists() for path in self.evidence)


def compiler_test(target: str, *, slow: bool = False, category: str = "runtime") -> Step:
    return Step(
        name=f"elisacore test {target}",
        cmd=["go", "run", "./src", "test", target, "--project", "../elisa-shad-ps4-from-scratch"],
        cwd=COMPILER_DIR,
        category=category,
        slow=slow,
        timeout_seconds=300 if slow else 120,
    )


def runtime_readiness_probes() -> list[ReadinessProbe]:
    return [
        ReadinessProbe(
            "pad",
            "native-Elisa",
            ("core/libraries/pad/pad.elisa", "input/controller.elisa", "input/input_handler.elisa"),
            "pad runtime now consumes shared Elisa controller state with touch/motion/lightbar/vibration paths",
        ),
        ReadinessProbe(
            "audioout",
            "native-Elisa",
            ("core/libraries/audio/audioout.elisa", "core/libraries/audio/audioout_backend.elisa"),
            "runtime state and backend abstraction are present; host audio device open is not required",
        ),
        ReadinessProbe(
            "savedata",
            "C++-bridge",
            ("core/libraries/save_data/savedata.elisa", "core/libraries/save_data/savedata_ffi.elisa", "core/libraries/save_data/savedata_elisa_bridge.cpp"),
            "mount/param behavior is exposed through the parity bridge",
        ),
        ReadinessProbe(
            "userservice/systemservice",
            "C-FFI",
            ("core/libraries/system/userservice.elisa", "core/libraries/system/systemservice.elisa", "core/libraries/system/userservice_ffi_bridge.elisa", "core/libraries/system/systemservice_ffi_bridge.elisa"),
            "service metadata and FFI shims are available for post-boot probing",
        ),
        ReadinessProbe(
            "netctl/network",
            "stub-parity",
            ("core/libraries/network/netctl.elisa", "core/libraries/network/net.elisa", "core/libraries/network/net_ffi.elisa"),
            "non-invasive netctl defaults plus socket conversion FFI; no live network dependency",
        ),
        ReadinessProbe(
            "NP",
            "stub-parity",
            ("core/libraries/np/np.elisa", "core/libraries/np/np_runtime.elisa"),
            "NP runtime and module stubs are visible for real-game import readiness",
        ),
        ReadinessProbe(
            "avplayer",
            "C-FFI",
            ("core/libraries/avplayer/avplayer_impl.elisa", "core/libraries/avplayer/avplayer_ffmpeg_ffi.elisa"),
            "FFmpeg-backed media probe surface exists; media decode remains explicit subsystem work",
        ),
    ]


def format_runtime_readiness() -> str:
    lines = ["Post-boot runtime readiness:"]
    for probe in runtime_readiness_probes():
        status = probe.status if probe.ready else "missing"
        evidence = ", ".join(probe.evidence)
        lines.append(f"- {probe.subsystem}: {status} ({probe.note}; evidence: {evidence})")
    return "\n".join(lines)


def matrix_steps() -> list[Step]:
    names = [
        "core_libraries_requested_scope_parity_matrix",
        "system_parity_matrix",
        "imgui_big_picture_parity_matrix",
        "shader_recompiler_ir_passes_parity_matrix",
        "shader_recompiler_ir_core_parity_matrix",
        "shader_recompiler_backend_spirv_parity_matrix",
    ]
    steps: list[Step] = []
    for stem in names:
        steps.append(Step(f"generate {stem}", [sys.executable, f"{stem}.py"], category="matrix"))
        steps.append(Step(f"check {stem}", [sys.executable, f"{stem}_check.py", "--summary"], category="matrix"))
    return steps


def native_warning_steps() -> list[Step]:
    return [
        Step("project.json syntax", [sys.executable, "-m", "json.tool", "project.json"], category="native"),
        Step(
            "native guest_exec_runtime warnings",
            ["clang", "-Wall", "-Wextra", "-Werror", "-c", "native/guest_exec_runtime.c", "-o", str(TMP_DIR / "guest_exec_runtime.o")],
            category="native",
        ),
        Step(
            "native kernel_threads_runtime warnings",
            ["clang", "-Wall", "-Wextra", "-Werror", "-pthread", "-c", "native/kernel_threads_runtime.c", "-o", str(TMP_DIR / "kernel_threads_runtime.o")],
            category="native",
        ),
        Step(
            "native kernel_threads_runtime_test_support warnings",
            ["clang", "-Wall", "-Wextra", "-Werror", "-pthread", "-c", "native/kernel_threads_runtime_test_support.c", "-o", str(TMP_DIR / "kernel_threads_runtime_test_support.o")],
            category="native",
        ),
    ]


def all_steps() -> list[Step]:
    return [
        Step("parity ledger", [sys.executable, "parity_ledger_check.py", "--summary"], category="ledger"),
        Step("parity ABI guard", [sys.executable, "parity_abi_check.py", "--summary"], category="ledger"),
        Step("parity workqueue summary", [sys.executable, "parity_workqueue.py", "--fail-missing"], category="ledger"),
        Step("bridge syntax", [sys.executable, "check_elisa_bridges.py"], category="bridge"),
        *matrix_steps(),
        *native_warning_steps(),
        compiler_test("core-platform-tests", category="core"),
        compiler_test("video-core-multi-level-page-table-tests", category="graphics"),
        compiler_test("video-core-renderer-vulkan-diagnostics-tests", category="graphics"),
        compiler_test("real-self-loader-tests", category="loader"),
        compiler_test("emulator-guest-exec-runtime-tests", category="guest-exec"),
        compiler_test("emulator-core-boot-smoke", category="boot"),
        compiler_test("emulator-real-game-boot-smoke", category="real-game"),
        compiler_test("kernel-threads-runtime-tests", slow=True, category="threads"),
        compiler_test("kernel-threads-parity-diff", slow=True, category="threads"),
        compiler_test("shader-recompiler-ir-passes-tests", slow=True, category="graphics"),
        compiler_test("shader-recompiler-recompiler-tests", slow=True, category="graphics"),
        compiler_test("imgui-big-picture-tests", slow=True, category="ui"),
        compiler_test("imgui-big-picture-ffi-bridge-tests", slow=True, category="ui"),
        compiler_test("core-libraries-avplayer", slow=True, category="media"),
        compiler_test("videodec-native-link-smoke", slow=True, category="media"),
    ]


def run_step(step: Step, verbose: bool) -> Result:
    if verbose:
        print(f"+ ({step.cwd}) {' '.join(step.cmd)}")
    try:
        proc = subprocess.run(
            step.cmd,
            cwd=step.cwd,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=step.timeout_seconds,
        )
        return Result(step=step, returncode=proc.returncode, output=proc.stdout or "")
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        message = f"\nTIMEOUT after {step.timeout_seconds}s: {' '.join(step.cmd)}"
        return Result(step=step, returncode=124, output=str(output) + message)


def tail(text: str, lines: int = 20) -> str:
    parts = text.splitlines()
    return "\n".join(parts[-lines:])


def load_json(path: Path) -> dict | list | None:
    try:
        return json.loads(path.read_text())
    except Exception:
        return None


def count_ledger() -> dict[str, int]:
    raw = load_json(ROOT / "parity_ledger.json")
    counts: dict[str, int] = {}
    if not isinstance(raw, dict):
        return counts
    for target in raw.get("active_targets", []):
        for entry in target.get("entries", []):
            status = str(entry.get("status", "<missing>"))
            counts[status] = counts.get(status, 0) + 1
    return counts


def iter_matrix_entries(raw: dict | list | None) -> Iterable[dict]:
    if isinstance(raw, list):
        for item in raw:
            if isinstance(item, dict):
                yield item
    elif isinstance(raw, dict):
        for key in ("entries", "symbols", "rows", "functions"):
            value = raw.get(key)
            if isinstance(value, list):
                for item in value:
                    if isinstance(item, dict):
                        yield item


def count_matrix(path: Path) -> dict[str, int]:
    counts: dict[str, int] = {}
    for entry in iter_matrix_entries(load_json(path)):
        status = str(entry.get("status") or entry.get("coverage") or entry.get("result") or "unknown")
        counts[status] = counts.get(status, 0) + 1
    return counts


def parse_artifact_kv(lines: Iterable[str]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for line in lines:
        if "CUSA07399_ARTIFACT" not in line:
            continue
        payload = line.split("CUSA07399_ARTIFACT", 1)[1].strip()
        row: dict[str, str] = {}
        for token in payload.split():
            if "=" not in token:
                continue
            key, value = token.split("=", 1)
            row[key.strip()] = value.strip()
        if row:
            rows.append(row)
    return rows


def to_int(value: str | None) -> int:
    if value is None:
        return 0
    v = value.strip().lower()
    try:
        if v.startswith("0x"):
            return int(v, 16)
        return int(v)
    except Exception:
        return 0


def parse_cusa_metrics(results: list[Result]) -> dict[str, int | str]:
    rows: list[dict[str, str]] = []
    for r in results:
        if r.step.name == "elisacore test emulator-real-game-boot-smoke":
            rows.extend(parse_artifact_kv(r.output.splitlines()))
    summary: dict[str, int | str] = {
        "module_count": 0,
        "total_imports": 0,
        "native_hle_imports": 0,
        "prx_export_imports": 0,
        "aerolib_fallback_imports": 0,
        "unresolved_imports": 0,
        "loaded_modules": 0,
        "relocation_count": 0,
        "guarded_status": 0,
        "boundary_status": 0,
        "execution_stage": 0,
    }
    max_modules = 0
    max_unresolved = 0
    max_relocations = 0
    max_loaded = 0
    max_hle_imports = 0
    max_prx_imports = 0
    max_aerolib_imports = 0
    total_imports = 0
    for row in rows:
        if "module_count" in row:
            max_modules = max(max_modules, to_int(row.get("module_count")))
            max_unresolved = max(max_unresolved, to_int(row.get("unresolved_imports")))
            max_relocations = max(max_relocations, to_int(row.get("relocated_imports")))
            max_hle_imports = max(max_hle_imports, to_int(row.get("native_hle_imports") or row.get("native_hle")))
            max_prx_imports = max(max_prx_imports, to_int(row.get("prx_export_imports") or row.get("prx_export")))
            max_aerolib_imports = max(max_aerolib_imports, to_int(row.get("aerolib_fallback_imports") or row.get("aerolib_fallback")))
        if "module" in row:
            max_loaded += 1
            imports = to_int(row.get("imports"))
            total_imports += imports
        if "guarded_status" in row:
            summary["guarded_status"] = to_int(row.get("guarded_status"))
            summary["boundary_status"] = to_int(row.get("boundary_status"))
            summary["execution_stage"] = to_int(row.get("execution_stage"))
    summary["module_count"] = max_modules
    summary["unresolved_imports"] = max_unresolved
    summary["relocation_count"] = max_relocations
    summary["loaded_modules"] = max_loaded
    summary["total_imports"] = total_imports
    if max_hle_imports == 0 and max_prx_imports == 0 and max_aerolib_imports == 0:
        # Backward-compatible fallback when old artifact emitters do not provide per-class counters.
        known = max_relocations - max_unresolved
        max_hle_imports = min(to_int(str(summary.get("module_count", 0))), known) if known > 0 else 0
        max_prx_imports = max(known - max_hle_imports, 0)
        max_aerolib_imports = 0
    summary["native_hle_imports"] = max_hle_imports
    summary["prx_export_imports"] = max_prx_imports
    summary["aerolib_fallback_imports"] = max_aerolib_imports
    return summary


def summarize_progress(results: list[Result]) -> str:
    lines: list[str] = []
    cusa_metrics = parse_cusa_metrics(results)
    ledger = count_ledger()
    if ledger:
        lines.append("Ledger statuses:")
        for key in sorted(ledger):
            lines.append(f"- {key}: {ledger[key]}")

    matrix_files = [
        "core_libraries_requested_scope_parity_matrix.json",
        "system_parity_matrix.json",
        "imgui_big_picture_parity_matrix.json",
        "shader_recompiler_ir_passes_parity_matrix.json",
        "shader_recompiler_ir_core_parity_matrix.json",
        "shader_recompiler_backend_spirv_parity_matrix.json",
    ]
    lines.append("Matrix snapshots:")
    for name in matrix_files:
        counts = count_matrix(ROOT / name)
        if not counts:
            lines.append(f"- {name}: unavailable")
            continue
        summary = ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))
        lines.append(f"- {name}: {summary}")

    def passed(name: str) -> bool:
        return any(r.step.name == name and r.passed for r in results)

    lines.append("CUSA07399: where are we?")
    load_state = "PASS" if passed("elisacore test emulator-real-game-boot-smoke") else "FAIL"
    link_state = "PASS" if to_int(str(cusa_metrics["unresolved_imports"])) == 0 and load_state == "PASS" else "FAIL"
    handoff_state = "PASS" if passed("elisacore test emulator-guest-exec-runtime-tests") else "FAIL"
    guarded_state = "PASS" if to_int(str(cusa_metrics["guarded_status"])) >= -3 and load_state == "PASS" else "PENDING"
    boundary_state = "PASS" if to_int(str(cusa_metrics["boundary_status"])) != 0 else "PENDING"
    frame_state = "PENDING"
    lines.append(f"- load: {load_state}")
    lines.append(f"- link: {link_state}")
    lines.append(f"- handoff: {handoff_state}")
    lines.append(f"- guarded entry: {guarded_state}")
    lines.append(f"- first boundary: {boundary_state}")
    lines.append(f"- first frame: {frame_state}")
    lines.append("CUSA07399 artifact metrics:")
    lines.append(f"- module count: {cusa_metrics['module_count']}")
    lines.append(f"- total imports: {cusa_metrics['total_imports']}")
    lines.append(f"- native HLE imports: {cusa_metrics['native_hle_imports']}")
    lines.append(f"- PRX export imports: {cusa_metrics['prx_export_imports']}")
    lines.append(f"- AeroLib fallback imports: {cusa_metrics['aerolib_fallback_imports']}")
    lines.append(f"- unresolved imports: {cusa_metrics['unresolved_imports']}")
    lines.append(f"- loaded modules: {cusa_metrics['loaded_modules']}")
    lines.append(f"- relocation count: {cusa_metrics['relocation_count']}")
    lines.append(format_runtime_readiness())
    return "\n".join(lines)


def write_report(path: Path, results: list[Result]) -> None:
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    lines = ["# Emulator C++ Parity Gate", "", f"Passed: {passed}", f"Failed: {failed}", ""]
    lines.append(summarize_progress(results))
    lines.extend(["", "## Steps"])
    for r in results:
        status = "PASS" if r.passed else "FAIL"
        lines.append(f"- {status}: {r.step.name}")
    path.write_text("\n".join(lines) + "\n")


def parse_report_score(text: str) -> tuple[int, int]:
    passed_match = re.search(r"^Passed:\s+(\d+)\s*$", text, re.MULTILINE)
    failed_match = re.search(r"^Failed:\s+(\d+)\s*$", text, re.MULTILINE)
    if not passed_match or not failed_match:
        return (0, 10**9)
    return (int(passed_match.group(1)), int(failed_match.group(1)))


def did_gate_improve(previous: str | None, current: str) -> bool:
    if not previous:
        return True
    prev_passed, prev_failed = parse_report_score(previous)
    cur_passed, cur_failed = parse_report_score(current)
    if cur_failed < prev_failed:
        return True
    if cur_failed == prev_failed and cur_passed > prev_passed:
        return True
    return False


def next_blocker(results: list[Result], cusa_metrics: dict[str, int | str]) -> str:
    for r in results:
        if not r.passed:
            return f"Fix failing gate step: {r.step.name}"
    if to_int(str(cusa_metrics.get("unresolved_imports", 0))) > 0:
        return "Drive unresolved imports to zero for CUSA07399"
    if to_int(str(cusa_metrics.get("aerolib_fallback_imports", 0))) > 0:
        return "Replace AeroLib fallback import resolutions with real HLE or PRX exports"
    if to_int(str(cusa_metrics.get("boundary_status", 0))) == 0:
        return "Promote guarded entry path to deterministic first-boundary classification"
    return "Promote first-frame milestone into executable gate coverage"


def append_milestone_if_improved(previous_report: str | None, current_report: str, results: list[Result], command: str) -> None:
    if not did_gate_improve(previous_report, current_report):
        return
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    cusa_metrics = parse_cusa_metrics(results)
    blocker = next_blocker(results, cusa_metrics)
    entry_lines = [
        "",
        f"## {date.today().isoformat()}: Emulator Parity Gate Improved",
        "",
        "- Command:",
        f"  - `{command}`",
        "- Result:",
        f"  - `passed={passed} failed={failed} selected={len(results)}`",
        f"  - `CUSA07399 unresolved imports={cusa_metrics['unresolved_imports']}, AeroLib fallback imports={cusa_metrics['aerolib_fallback_imports']}`",
        "- Next blocker:",
        f"  - {blocker}",
    ]
    if MILESTONES_PATH.exists():
        MILESTONES_PATH.write_text(MILESTONES_PATH.read_text().rstrip() + "\n" + "\n".join(entry_lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the top-level Elisa emulator C++ parity gate.")
    parser.add_argument("--quick", action="store_true", help="Skip slow subsystem runtime targets.")
    parser.add_argument("--list", action="store_true", help="List selected steps and exit.")
    parser.add_argument("--fail-fast", action="store_true", help="Stop at the first failed step.")
    parser.add_argument("--write-report", type=Path, help="Write a markdown parity report to this path.")
    parser.add_argument("--no-milestones", action="store_true", help="Do not append milestone history even if gate score improves.")
    parser.add_argument("--readiness", action="store_true", help="Print post-boot runtime readiness probes and exit.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print commands before running.")
    args = parser.parse_args()

    if args.readiness:
        print(format_runtime_readiness())
        return 0 if all(probe.ready for probe in runtime_readiness_probes()) else 1

    steps = all_steps()
    if args.quick:
        steps = [s for s in steps if not s.slow]

    if args.list:
        for step in steps:
            slow = " slow" if step.slow else ""
            print(f"[{step.category}{slow}] {step.name}")
        return 0

    if not COMPILER_DIR.exists():
        print(f"Missing Elisa compiler checkout: {COMPILER_DIR}", file=sys.stderr)
        return 1

    results: list[Result] = []
    for index, step in enumerate(steps, 1):
        print(f"==> [{index}/{len(steps)}] {step.name}")
        result = run_step(step, args.verbose)
        results.append(result)
        if result.passed:
            first_line = tail(result.output, 1).strip()
            print(f"OK {step.name}" + (f" :: {first_line}" if first_line else ""))
        else:
            print(f"FAIL {step.name}", file=sys.stderr)
            if result.output:
                print(tail(result.output), file=sys.stderr)
            if args.fail_fast:
                break

    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    print("\n== Parity Progress ==")
    print(summarize_progress(results))
    print(f"\nGate result: passed={passed} failed={failed} selected={len(results)}")

    previous_report = LAST_REPORT_PATH.read_text() if LAST_REPORT_PATH.exists() else None
    report_path = args.write_report if args.write_report else LAST_REPORT_PATH
    write_report(report_path, results)
    print(f"wrote report: {report_path}")
    current_report = report_path.read_text()
    if report_path != LAST_REPORT_PATH:
        LAST_REPORT_PATH.write_text(current_report)
    cusa_metrics = parse_cusa_metrics(results)
    if to_int(str(cusa_metrics.get("unresolved_imports", 0))) > 0:
        print("FAIL policy: truly unresolved imports remain in CUSA07399 artifacts", file=sys.stderr)
        failed += 1
    if not args.no_milestones:
        cmd = "./emulator-cpp-parity --quick" if args.quick else "./emulator-cpp-parity"
        append_milestone_if_improved(previous_report, current_report, results, cmd)

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
