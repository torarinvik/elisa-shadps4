#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"
TMP_DIR = Path(os.environ.get("TMPDIR", "/tmp"))


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
            "C++-bridge",
            ("core/libraries/pad/pad_ffi.elisa", "core/libraries/pad/pad_elisa_bridge.cc"),
            "probe-only controller surface; device/live SDL state remains bridge-backed",
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
        Step("parity workqueue summary", [sys.executable, "parity_workqueue.py"], category="ledger"),
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


def summarize_progress(results: list[Result]) -> str:
    lines: list[str] = []
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

    lines.append("CUSA07399 staged gate:")
    if passed("elisacore test emulator-real-game-boot-smoke"):
        lines.append("- load/link/handoff: PASS")
        if passed("elisacore test emulator-guest-exec-runtime-tests"):
            lines.append("- execute-fixture: PASS")
            machine = platform.machine().lower()
            if machine in {"x86_64", "amd64"}:
                lines.append("- execute-game: ARMED_PROBE_ONLY")
            else:
                lines.append(f"- execute-game: UNSUPPORTED_HOST ({machine})")
            lines.append("- frame: not yet promoted into this gate")
        else:
            lines.append("- execute-fixture: FAIL or not run")
            lines.append("- execute-game/frame: not yet promoted into this gate")
    else:
        lines.append("- load/link/handoff: FAIL or not run")
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the top-level Elisa emulator C++ parity gate.")
    parser.add_argument("--quick", action="store_true", help="Skip slow subsystem runtime targets.")
    parser.add_argument("--list", action="store_true", help="List selected steps and exit.")
    parser.add_argument("--fail-fast", action="store_true", help="Stop at the first failed step.")
    parser.add_argument("--write-report", type=Path, help="Write a markdown parity report to this path.")
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

    if args.write_report:
        write_report(args.write_report, results)
        print(f"wrote report: {args.write_report}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
