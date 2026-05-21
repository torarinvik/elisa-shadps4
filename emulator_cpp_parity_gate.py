#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.util
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
LEDGER_PATH = ROOT / "parity_ledger.json"
PROJECT_PATH = ROOT / "project.json"
CUSA_PATH = ROOT.parent / "shadPS4" / "Games" / "CUSA07399"
CUSA_ARTIFACT_PATH = ROOT / "cusa07399_artifacts.txt"


@dataclass(frozen=True)
class Step:
    name: str
    cmd: list[str]
    cwd: Path = ROOT
    category: str = "gate"
    slow: bool = False
    timeout_seconds: int | None = None


@dataclass
class Result:
    step: Step
    returncode: int
    output: str

    @property
    def passed(self) -> bool:
        return self.returncode == 0


def compiler_test(target: str, *, slow: bool = False, category: str = "runtime") -> Step:
    return Step(
        name=f"elisacore test {target}",
        cmd=["go", "run", "./src", "test", target, "--project", "../elisa-shad-ps4-from-scratch"],
        cwd=COMPILER_DIR,
        category=category,
        slow=slow,
        timeout_seconds=300 if slow else 120,
    )


def compiler_test_source(source: str, *, slow: bool = False, category: str = "runtime") -> Step:
    return Step(
        name=f"elisacore run {source}",
        cmd=["go", "run", "./src", f"../elisa-shad-ps4-from-scratch/{source}"],
        cwd=COMPILER_DIR,
        category=category,
        slow=slow,
        timeout_seconds=300 if slow else 120,
    )


def host_audio_backends_present() -> bool:
    if sys.platform != "darwin":
        return False
    sdl_candidates = [
        Path("/opt/homebrew/lib/libSDL3.dylib"),
        Path("/opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib"),
        Path("/usr/local/lib/libSDL3.dylib"),
    ]
    openal_candidates = [
        Path("/System/Library/Frameworks/OpenAL.framework/OpenAL"),
        Path("/System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL"),
    ]
    return any(path.exists() for path in sdl_candidates) and any(path.exists() for path in openal_candidates)


def all_steps() -> list[Step]:
    guest_exec_warning_cmd = ["clang", "-Wall", "-Wextra", "-Werror", "-c", "native/guest_exec_runtime.c", "-o", str(TMP_DIR / "guest_exec_runtime.o")]
    if sys.platform == "darwin":
        guest_exec_warning_cmd.insert(1, "-DMAP_ANONYMOUS=MAP_ANON")
    steps = [
        Step("project.json syntax", [sys.executable, "-m", "json.tool", str(PROJECT_PATH)], category="validation"),
        Step("parity ledger", [sys.executable, "parity_ledger_check.py", "--summary"], category="ledger"),
        Step("parity ABI guard", [sys.executable, "parity_abi_check.py", "--summary"], category="ledger"),
        Step("parity workqueue summary", [sys.executable, "parity_workqueue.py", "--fail-missing"], category="ledger"),
        Step("bridge syntax", [sys.executable, "check_elisa_bridges.py"], category="bridge"),
        compiler_test("core-libraries-audio-parity-tests", category="audio"),
        Step(
            "native guest_exec_runtime warnings",
            guest_exec_warning_cmd,
            category="native",
        ),
        Step(
            "native kernel_threads_runtime warnings",
            ["clang", "-Wall", "-Wextra", "-Werror", "-pthread", "-c", "native/kernel_threads_runtime.c", "-o", str(TMP_DIR / "kernel_threads_runtime.o")],
            category="native",
        ),
        compiler_test("emulator-core-boot-smoke", category="boot"),
        compiler_test("real-self-loader-tests", category="loader"),
        compiler_test("emulator-real-game-boot-smoke", category="cusa"),
        compiler_test("emulator-guest-exec-runtime-tests", category="guest-exec"),
        compiler_test("kernel-threads-runtime-tests", slow=True, category="threads"),
        compiler_test("video-core-renderer-vulkan-diagnostics-tests", slow=True, category="graphics"),
        compiler_test("video-core-multi-level-page-table-tests", slow=True, category="graphics"),
        compiler_test("core-libraries-avplayer", slow=True, category="audio-input-system"),
        compiler_test("core-libraries-pad-tests", slow=True, category="audio-input-system"),
    ]
    if host_audio_backends_present():
        steps.append(compiler_test_source("core_libraries_audioout_isolated.elisa", slow=True, category="audio"))
    return steps


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
    return "\n".join(text.splitlines()[-lines:])


def load_json(path: Path) -> dict | list | None:
    try:
        return json.loads(path.read_text())
    except Exception:
        return None


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


def gather_artifact_rows(results: list[Result]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for r in results:
        rows.extend(parse_artifact_kv(r.output.splitlines()))
    if CUSA_ARTIFACT_PATH.exists():
        rows.extend(parse_artifact_kv(CUSA_ARTIFACT_PATH.read_text().splitlines()))
    return rows


def parse_cusa_metrics(results: list[Result]) -> dict[str, int | str]:
    rows = gather_artifact_rows(results)
    summary: dict[str, int | str] = {
        "module_count": 0,
        "total_imports": 0,
        "native_hle_imports": 0,
        "prx_export_imports": 0,
        "aerolib_fallback_imports": 0,
        "unresolved_imports": 0,
        "malformed_imports": 0,
        "audio_sdl_available": 0,
        "audio_openal_available": 0,
        "audio_output_opened": 0,
        "audio_input_opened": 0,
        "relocation_count": 0,
        "execution_stage": 0,
        "boundary_status": 0,
        "last_hle_module": "",
        "last_hle_symbol": "",
        "guest_exec_started": 0,
        "guest_exec_entry_reached": 0,
        "guest_exec_first_boundary_reached": 0,
        "guest_exec_boundary_reason": 0,
        "guest_exec_last_pc": 0,
        "guest_exec_last_signal": 0,
        "video_stage_opened": 0,
        "video_stage_buffers_registered": 0,
        "video_stage_flip_submitted": 0,
        "video_stage_flip_completed": 0,
        "video_stage_first_frame_candidate": 0,
        "shader_translate_attempted": 0,
        "shader_translate_path_bridge": 0,
        "shader_translate_path_native": 0,
    }
    def row_has_kv(row: dict[str, str], key: str) -> bool:
        if to_int(row.get(key)) > 0:
            return True
        if row.get("key") == key and to_int(row.get("value")) > 0:
            return True
        return False
    for row in rows:
        summary["module_count"] = max(int(summary["module_count"]), to_int(row.get("module_count")))
        summary["total_imports"] = max(int(summary["total_imports"]), to_int(row.get("total_imports")))
        summary["native_hle_imports"] = max(int(summary["native_hle_imports"]), to_int(row.get("native_hle_imports") or row.get("native_hle")))
        summary["prx_export_imports"] = max(int(summary["prx_export_imports"]), to_int(row.get("prx_export_imports") or row.get("prx_export")))
        summary["aerolib_fallback_imports"] = max(int(summary["aerolib_fallback_imports"]), to_int(row.get("aerolib_fallback_imports") or row.get("aerolib_fallback")))
        summary["unresolved_imports"] = max(int(summary["unresolved_imports"]), to_int(row.get("unresolved_imports")))
        summary["malformed_imports"] = max(int(summary["malformed_imports"]), to_int(row.get("malformed_imports")))
        summary["audio_sdl_available"] = max(int(summary["audio_sdl_available"]), to_int(row.get("audio_sdl_available")))
        summary["audio_openal_available"] = max(int(summary["audio_openal_available"]), to_int(row.get("audio_openal_available")))
        summary["audio_output_opened"] = max(int(summary["audio_output_opened"]), to_int(row.get("audio_output_opened")))
        summary["audio_input_opened"] = max(int(summary["audio_input_opened"]), to_int(row.get("audio_input_opened")))
        summary["relocation_count"] = max(int(summary["relocation_count"]), to_int(row.get("relocated_imports")))
        summary["execution_stage"] = max(int(summary["execution_stage"]), to_int(row.get("execution_stage")))
        summary["boundary_status"] = max(int(summary["boundary_status"]), to_int(row.get("boundary_status")))
        if row.get("last_hle_module"):
            summary["last_hle_module"] = row.get("last_hle_module", "")
        if row.get("last_hle_symbol"):
            summary["last_hle_symbol"] = row.get("last_hle_symbol", "")
        if row.get("guest_exec_started") is not None:
            summary["guest_exec_started"] = max(int(summary["guest_exec_started"]), to_int(row.get("guest_exec_started")))
        if row.get("guest_exec_entry_reached") is not None:
            summary["guest_exec_entry_reached"] = max(int(summary["guest_exec_entry_reached"]), to_int(row.get("guest_exec_entry_reached")))
        if row.get("guest_exec_first_boundary_reached") is not None:
            summary["guest_exec_first_boundary_reached"] = max(int(summary["guest_exec_first_boundary_reached"]), to_int(row.get("guest_exec_first_boundary_reached")))
        if row.get("guest_exec_boundary_reason") is not None:
            summary["guest_exec_boundary_reason"] = to_int(row.get("guest_exec_boundary_reason"))
        if row.get("guest_exec_last_pc") is not None:
            summary["guest_exec_last_pc"] = max(int(summary["guest_exec_last_pc"]), to_int(row.get("guest_exec_last_pc")))
        if row.get("guest_exec_last_signal") is not None:
            summary["guest_exec_last_signal"] = max(int(summary["guest_exec_last_signal"]), to_int(row.get("guest_exec_last_signal")))
        if row_has_kv(row, "VIDEO_STAGE_opened"):
            summary["video_stage_opened"] = 1
        if row_has_kv(row, "VIDEO_STAGE_buffers_registered"):
            summary["video_stage_buffers_registered"] = 1
        if row_has_kv(row, "VIDEO_STAGE_flip_submitted"):
            summary["video_stage_flip_submitted"] = 1
        if row_has_kv(row, "VIDEO_STAGE_flip_completed"):
            summary["video_stage_flip_completed"] = 1
        if row_has_kv(row, "VIDEO_STAGE_first_frame_candidate"):
            summary["video_stage_first_frame_candidate"] = 1
        if row_has_kv(row, "shader_translate_attempted"):
            summary["shader_translate_attempted"] = 1
        if row_has_kv(row, "shader_translate_path_bridge"):
            summary["shader_translate_path_bridge"] = 1
        if row_has_kv(row, "shader_translate_path_native"):
            summary["shader_translate_path_native"] = 1
    return summary


def parse_audio_skip_reason(results: list[Result]) -> str:
    prefix = "AUDIO_RUNTIME_SKIP"
    for r in results:
        for line in r.output.splitlines():
            text = line.strip()
            if text.startswith(prefix):
                return text.removeprefix(prefix).strip()
    return ""


def subsystem_guess(library: str, module: str, nid: str) -> str:
    probe = f"{library} {module} {nid}".lower()
    if "video" in probe or "gnm" in probe or "vulkan" in probe:
        return "graphics"
    if "audio" in probe:
        return "audio_input_service"
    if "pad" in probe or "controller" in probe or "hmd" in probe or "ime" in probe:
        return "audio_input_service"
    if "kernel" in probe or "loader" in probe or "exec" in probe:
        return "loader_guest_exec"
    return "kernel"


def fallback_symbols(results: list[Result]) -> list[dict[str, str | int]]:
    rows = gather_artifact_rows(results)
    out: list[dict[str, str | int]] = []
    for row in rows:
        if row.get("resolution") != "3":
            continue
        if row.get("fallback_import") is None:
            continue
        nid = row.get("nid", "")
        library = row.get("library", "")
        module = row.get("module", "")
        source = row.get("source", "")
        subsystem = row.get("subsystem", "")
        if subsystem == "":
            subsystem = subsystem_guess(library, module, nid)
        out.append(
            {
                "nid": nid,
                "library": library,
                "module": module,
                "source": source,
                "subsystem": subsystem,
            }
        )
    counts: dict[tuple[str, str, str, str, str], int] = {}
    for r in out:
        key = (str(r["nid"]), str(r["library"]), str(r["module"]), str(r["source"]), str(r["subsystem"]))
        counts[key] = counts.get(key, 0) + 1
    rows2 = [
        {"nid": k[0], "library": k[1], "module": k[2], "source": k[3], "subsystem": k[4], "count": c}
        for k, c in counts.items()
    ]
    rows2.sort(key=lambda item: int(item["count"]), reverse=True)
    return rows2


def stage_name(stage: int) -> str:
    mapping = {
        0: "none",
        1: "load",
        2: "link",
        3: "handoff",
        4: "guarded-entry",
        5: "first-boundary",
        6: "crash-captured",
    }
    return mapping.get(stage, f"unknown({stage})")


def cusa_stage_summary(results: list[Result], cusa: dict[str, int | str]) -> dict[str, str]:
    def passed(name: str) -> bool:
        return any(r.step.name == name and r.passed for r in results)

    load = "PASS" if passed("elisacore test emulator-real-game-boot-smoke") else "FAIL"
    link = "PASS" if load == "PASS" and int(cusa["unresolved_imports"]) == 0 else "FAIL"
    handoff = "PASS" if passed("elisacore test emulator-guest-exec-runtime-tests") else "FAIL"
    exec_stage = stage_name(int(cusa["execution_stage"]))
    boundary = "PASS" if int(cusa["boundary_status"]) != 0 else "PENDING"
    frame_gate = (
        int(cusa["boundary_status"]) != 0
        and
        int(cusa["execution_stage"]) >= 4
        and int(cusa["video_stage_opened"]) == 1
        and int(cusa["video_stage_buffers_registered"]) == 1
        and int(cusa["video_stage_flip_submitted"]) == 1
        and int(cusa["video_stage_flip_completed"]) == 1
        and int(cusa["video_stage_first_frame_candidate"]) == 1
        and int(cusa["shader_translate_attempted"]) == 1
        and (int(cusa["shader_translate_path_bridge"]) == 1 or int(cusa["shader_translate_path_native"]) == 1)
    )
    frame = "PASS" if frame_gate else "PENDING"
    frame_ladder = "none"
    if int(cusa["video_stage_opened"]) == 1:
        frame_ladder = "opened"
    if int(cusa["video_stage_buffers_registered"]) == 1:
        frame_ladder = "opened -> buffers_registered"
    if int(cusa["video_stage_flip_submitted"]) == 1:
        frame_ladder = f"{frame_ladder} -> flip_submitted"
    if int(cusa["video_stage_flip_completed"]) == 1:
        frame_ladder = f"{frame_ladder} -> flip_completed"
    if int(cusa["video_stage_first_frame_candidate"]) == 1:
        frame_ladder = f"{frame_ladder} -> first_frame_candidate"
    return {
        "load": load,
        "link": link,
        "handoff": handoff,
        "execute": exec_stage,
        "boundary": boundary,
        "frame": frame,
        "frame_ladder": frame_ladder,
    }


def host_exec_note() -> str:
    machine = platform.machine().lower()
    if sys.platform == "darwin" and machine in {"arm64", "aarch64"}:
        return "arm64 macOS remains probe-only unless a translation path exists"
    if sys.platform == "win32":
        return "Windows remains unsupported until the trampoline path exists"
    if machine in {"x86_64", "amd64"}:
        return "x86_64 hosts can attempt guarded guest execution"
    return f"host guest execution support is probe-only on {platform.system()} {platform.machine()}"


def load_workqueue_rows() -> list[dict[str, str]]:
    path = ROOT / "parity_workqueue.py"
    spec = importlib.util.spec_from_file_location("parity_workqueue", path)
    if spec is None or spec.loader is None:
        return []
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.build_rows()  # type: ignore[attr-defined]


def count_ledger_statuses() -> dict[str, int]:
    raw = load_json(LEDGER_PATH)
    counts: dict[str, int] = {}
    if not isinstance(raw, dict):
        return counts
    for target in raw.get("active_targets", []):
        for entry in target.get("entries", []):
            status = str(entry.get("status", "<missing>"))
            counts[status] = counts.get(status, 0) + 1
    return counts


def failing_scenario_ids(results: list[Result]) -> list[str]:
    ids: list[str] = []
    seen: set[str] = set()
    panic_re = re.compile(r"^\[\s*PANIC\s*\]\s+([A-Za-z0-9_./:-]+)")
    active_re = re.compile(r"^\[\s*ACTIVE\s*\]\s+([A-Za-z0-9_./:-]+)")
    for r in results:
        if r.passed:
            continue
        for line in r.output.splitlines():
            text = line.strip()
            m = panic_re.match(text) or active_re.match(text)
            if not m:
                continue
            sid = m.group(1)
            if sid in seen:
                continue
            seen.add(sid)
            ids.append(sid)
    if ids:
        return ids
    for r in results:
        if r.passed or not r.step.name.startswith("elisacore test "):
            continue
        sid = r.step.name.removeprefix("elisacore test ").strip()
        if sid and sid not in seen:
            seen.add(sid)
            ids.append(sid)
    return ids


def validate_rules(results: list[Result], cusa: dict[str, int | str], fallback_rows: list[dict[str, str | int]], require_first_boundary: bool) -> list[str]:
    errors: list[str] = []

    workqueue = load_workqueue_rows()
    unclassified = [r for r in workqueue if r.get("status") == "Missing"]
    if unclassified:
        errors.append(f"No unclassified project-owned C++ modules violated: {len(unclassified)} Missing units")

    if int(cusa.get("unresolved_imports", 0)) > 0:
        errors.append(f"No unresolved imports in CUSA gate violated: unresolved={cusa['unresolved_imports']}")
    if require_first_boundary:
        boundary_status = int(cusa.get("boundary_status", 0))
        first_boundary_reached = int(cusa.get("guest_exec_first_boundary_reached", 0))
        if boundary_status not in (1, -2) and first_boundary_reached == 0:
            errors.append("First-boundary probe requested but CUSA07399 did not reach the first guarded boundary")

    ledger_raw = load_json(LEDGER_PATH)
    if isinstance(ledger_raw, dict):
        for target in ledger_raw.get("active_targets", []):
            for entry in target.get("entries", []):
                if entry.get("status") == "Temporary-Cpp-Bridge":
                    if not entry.get("bridge_blocker") or not entry.get("retirement_plan") or not entry.get("retirement_criteria"):
                        errors.append(f"Temporary bridge missing blocker/retirement criteria: {target.get('name')}::{entry.get('symbol')}")
                if entry.get("status") == "Native-Elisa":
                    tests = entry.get("tests", [])
                    if not isinstance(tests, list) or len(tests) == 0:
                        errors.append(f"Native subsystem without test evidence: {target.get('name')}::{entry.get('symbol')}")

    queues = build_fallback_queues(fallback_rows)
    known = set()
    for items in queues.values():
        for item in items:
            known.add((item["nid"], item["library"], item["module"]))
    for row in fallback_rows:
        key = (str(row["nid"]), str(row["library"]), str(row["module"]))
        if key not in known:
            errors.append(f"Fallback symbol missing from queue: {row['nid']} {row['library']} {row['module']}")

    for r in results:
        if not r.passed:
            errors.append(f"Gate step failed: {r.step.name}")

    return errors


def build_fallback_queues(rows: list[dict[str, str | int]]) -> dict[str, list[dict[str, str | int]]]:
    queues = {
        "kernel_fallbacks": [],
        "graphics_fallbacks": [],
        "audio_input_service_fallbacks": [],
        "loader_guest_exec_blockers": [],
    }
    for row in rows:
        subsystem = str(row.get("subsystem", ""))
        if subsystem == "graphics":
            queues["graphics_fallbacks"].append(row)
        elif subsystem == "audio_input_service":
            queues["audio_input_service_fallbacks"].append(row)
        elif subsystem == "loader_guest_exec":
            queues["loader_guest_exec_blockers"].append(row)
        else:
            queues["kernel_fallbacks"].append(row)
    for key in queues:
        queues[key] = sorted(queues[key], key=lambda item: int(item.get("count", 0)), reverse=True)
    return queues


def score_summary(results: list[Result], cusa: dict[str, int | str], errors: list[str]) -> tuple[int, int]:
    total = len(results) + 5
    passed = sum(1 for r in results if r.passed)
    if int(cusa["unresolved_imports"]) == 0:
        passed += 1
    if int(cusa["module_count"]) > 0:
        passed += 1
    if int(cusa["execution_stage"]) >= 1:
        passed += 1
    if int(cusa["boundary_status"]) != 0:
        passed += 1
    if not errors:
        passed += 1
    return passed, total


def summarize_progress(results: list[Result], require_first_boundary: bool = False) -> tuple[str, dict[str, list[dict[str, str | int]]], dict[str, int | str], list[str]]:
    lines: list[str] = []
    ledger_counts = count_ledger_statuses()
    cusa = parse_cusa_metrics(results)
    fallback_rows = fallback_symbols(results)
    queues = build_fallback_queues(fallback_rows)
    rules = validate_rules(results, cusa, fallback_rows, require_first_boundary)
    stages = cusa_stage_summary(results, cusa)

    passed_score, total_score = score_summary(results, cusa, rules)
    native_count = ledger_counts.get("Native-Elisa", 0)
    bridge_count = ledger_counts.get("Temporary-Cpp-Bridge", 0)
    fallback_count = int(cusa["aerolib_fallback_imports"])
    unresolved_count = int(cusa["unresolved_imports"])
    missing_count = ledger_counts.get("Missing", 0)
    unverified_count = ledger_counts.get("Stub-Parity", 0) + ledger_counts.get("AeroLibFallback", 0)
    scenario_ids = failing_scenario_ids(results)
    lines.append(f"Summary score: {passed_score}/{total_score}")
    lines.append("Per-subsystem counts:")
    for status in sorted(ledger_counts):
        lines.append(f"- {status}: {ledger_counts[status]}")
    lines.append("Parity gate counts (native/bridge/fallback/unresolved):")
    lines.append(f"- native: {native_count}")
    lines.append(f"- bridge: {bridge_count}")
    lines.append(f"- fallback: {fallback_count}")
    lines.append(f"- unresolved: {unresolved_count}")
    lines.append("Ledger risk counts (missing/unverified):")
    lines.append(f"- missing: {missing_count}")
    lines.append(f"- unverified: {unverified_count}")

    lines.append("CUSA07399 stage:")
    lines.append(f"- execution stage raw: {cusa['execution_stage']}")
    lines.append(f"- load: {stages['load']}")
    lines.append(f"- link: {stages['link']}")
    lines.append(f"- handoff: {stages['handoff']}")
    lines.append(f"- execute stage: {stages['execute']}")
    lines.append(f"- first boundary: {stages['boundary']}")
    lines.append(f"- first frame: {stages['frame']}")
    lines.append(f"- first frame ladder: {stages['frame_ladder']}")
    lines.append(f"- host note: {host_exec_note()}")
    lines.append(f"- guest exec started: {cusa['guest_exec_started']}")
    lines.append(f"- guest exec entry reached: {cusa['guest_exec_entry_reached']}")
    lines.append(f"- guest exec first boundary reached: {cusa['guest_exec_first_boundary_reached']}")
    lines.append(f"- guest exec boundary reason: {cusa['guest_exec_boundary_reason']}")
    lines.append(f"- guest exec last pc: 0x{int(cusa['guest_exec_last_pc']):x}")
    lines.append(f"- guest exec last signal: {cusa['guest_exec_last_signal']}")
    lines.append("- first frame gate signals:")
    lines.append(f"  - shader_translate_attempted={cusa['shader_translate_attempted']}")
    lines.append(f"  - shader_path_bridge={cusa['shader_translate_path_bridge']} shader_path_native={cusa['shader_translate_path_native']}")
    lines.append(f"  - VIDEO_STAGE_opened={cusa['video_stage_opened']}")
    lines.append(f"  - VIDEO_STAGE_buffers_registered={cusa['video_stage_buffers_registered']}")
    lines.append(f"  - VIDEO_STAGE_flip_submitted={cusa['video_stage_flip_submitted']}")
    lines.append(f"  - VIDEO_STAGE_flip_completed={cusa['video_stage_flip_completed']}")
    lines.append(f"  - VIDEO_STAGE_first_frame_candidate={cusa['video_stage_first_frame_candidate']}")

    lines.append("CUSA07399 artifact metrics:")
    lines.append(f"- CUSA module count: {cusa['module_count']}")
    lines.append(f"- imports total: {cusa['total_imports']}")
    lines.append(f"- native HLE count: {cusa['native_hle_imports']}")
    lines.append(f"- PRX export count: {cusa['prx_export_imports']}")
    lines.append(f"- AeroLib fallback count: {cusa['aerolib_fallback_imports']}")
    lines.append(f"- unresolved count: {cusa['unresolved_imports']}")
    lines.append(f"- malformed count: {cusa['malformed_imports']}")
    lines.append(f"- audio SDL available: {cusa['audio_sdl_available']}")
    lines.append(f"- audio OpenAL available: {cusa['audio_openal_available']}")
    lines.append(f"- audio output opened: {cusa['audio_output_opened']}")
    lines.append(f"- audio input opened: {cusa['audio_input_opened']}")
    audio_skip_reason = parse_audio_skip_reason(results)
    if audio_skip_reason:
        lines.append(f"- audio runtime skip reason: {audio_skip_reason}")
    lines.append(f"- current execution stage: {cusa['execution_stage']} ({stage_name(int(cusa['execution_stage']))})")
    lines.append(f"- last HLE call: module={cusa['last_hle_module']} symbol={cusa['last_hle_symbol']}")
    lines.append(f"- current video/audio/input stage: graphics={len(queues['graphics_fallbacks'])} audio-input-service={len(queues['audio_input_service_fallbacks'])}")

    lines.append("Top 25 fallback symbols:")
    for item in fallback_rows[:25]:
        lines.append(
            f"- [{item['count']}] nid={item['nid']} lib={item['library']} module={item['module']} source={item['source']} subsystem={item['subsystem']}"
        )
    if not fallback_rows:
        lines.append("- none")

    lines.append("Top blockers:")
    if rules:
        for err in rules[:25]:
            lines.append(f"- {err}")
    else:
        lines.append("- none")
    lines.append("Failing scenario ids:")
    if scenario_ids:
        for sid in scenario_ids[:25]:
            lines.append(f"- {sid}")
    else:
        lines.append("- none")

    return "\n".join(lines), queues, cusa, rules


def write_report(path: Path, results: list[Result], progress: str, queues: dict[str, list[dict[str, str | int]]]) -> None:
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    lines = ["# Emulator C++ Parity Gate", "", f"Passed: {passed}", f"Failed: {failed}", "", progress, "", "## Agent Work Queues"]
    for qname, items in queues.items():
        lines.append(f"### {qname}")
        if not items:
            lines.append("- none")
            continue
        for item in items[:25]:
            lines.append(f"- [{item['count']}] {item['nid']} {item['library']} {item['module']} ({item['source']})")
    lines.extend(["", "## Steps"])
    for r in results:
        status = "PASS" if r.passed else "FAIL"
        lines.append(f"- {status}: {r.step.name}")
    path.write_text("\n".join(lines) + "\n")


def parse_report_score(text: str) -> tuple[int, int]:
    p = re.search(r"^Passed:\s+(\d+)\s*$", text, re.MULTILINE)
    f = re.search(r"^Failed:\s+(\d+)\s*$", text, re.MULTILINE)
    if not p or not f:
        return (0, 10**9)
    return (int(p.group(1)), int(f.group(1)))


def did_gate_improve(previous: str | None, current: str) -> bool:
    if not previous:
        return True
    pp, pf = parse_report_score(previous)
    cp, cf = parse_report_score(current)
    return cf < pf or (cf == pf and cp > pp)


def append_milestone_if_improved(previous_report: str | None, current_report: str, results: list[Result], command: str, cusa: dict[str, int | str], rules: list[str]) -> None:
    if not did_gate_improve(previous_report, current_report):
        return
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    improved = "none"
    if int(cusa["unresolved_imports"]) == 0:
        improved = "CUSA unresolved imports are zero"
    if failed == 0:
        improved = f"all {len(results)} selected gate steps passed"
    next_blocker = rules[0] if rules else "promote execute/boundary/frame stages deeper into runtime coverage"
    entry = [
        "",
        f"## {date.today().isoformat()}: Emulator Parity Gate Run",
        "",
        f"- Date: {date.today().isoformat()}",
        f"- Command run: `{command}`",
        f"- Result: `passed={passed} failed={failed} selected={len(results)}`",
        f"- What improved: {improved}",
        f"- Next blocker: {next_blocker}",
    ]
    if MILESTONES_PATH.exists():
        MILESTONES_PATH.write_text(MILESTONES_PATH.read_text().rstrip() + "\n" + "\n".join(entry) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the top-level Elisa emulator C++ parity gate.")
    parser.add_argument("--quick", action="store_true", help="Skip slow subsystem runtime targets.")
    parser.add_argument("--list", action="store_true", help="List selected steps and exit.")
    parser.add_argument("--fail-fast", action="store_true", help="Stop at first failed step.")
    parser.add_argument("--write-report", type=Path, help="Write report path.")
    parser.add_argument("--no-milestones", action="store_true", help="Do not append milestones.")
    parser.add_argument("--require-first-boundary", action="store_true", help="Fail when CUSA07399 does not reach the first guarded guest boundary.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose command output.")
    args = parser.parse_args()

    steps = all_steps()
    if not CUSA_PATH.exists():
        steps = [s for s in steps if s.category != "cusa"]
    if args.quick:
        steps = [s for s in steps if not s.slow]
    if args.list:
        for step in steps:
            print(f"[{step.category}{' slow' if step.slow else ''}] {step.name}")
        return 0

    if not COMPILER_DIR.exists():
        print(f"Missing Elisa compiler checkout: {COMPILER_DIR}", file=sys.stderr)
        return 1

    results: list[Result] = []
    for i, step in enumerate(steps, 1):
        print(f"==> [{i}/{len(steps)}] {step.name}")
        result = run_step(step, args.verbose)
        results.append(result)
        if result.passed:
            print(f"OK {step.name}")
        else:
            print(f"FAIL {step.name}", file=sys.stderr)
            if result.output:
                print(tail(result.output), file=sys.stderr)
            if args.fail_fast:
                break

    progress, queues, cusa, rules = summarize_progress(results, args.require_first_boundary)
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed

    print("\n== Parity Progress ==")
    print(progress)
    print(f"\nGate result: passed={passed} failed={failed} selected={len(results)}")

    previous_report = LAST_REPORT_PATH.read_text() if LAST_REPORT_PATH.exists() else None
    report_path = args.write_report if args.write_report else LAST_REPORT_PATH
    write_report(report_path, results, progress, queues)
    print(f"wrote report: {report_path}")
    current_report = report_path.read_text()
    if report_path != LAST_REPORT_PATH:
        LAST_REPORT_PATH.write_text(current_report)

    if not args.no_milestones:
        cmd = "./emulator-cpp-parity --quick" if args.quick else "./emulator-cpp-parity"
        append_milestone_if_improved(previous_report, current_report, results, cmd, cusa, rules)

    # policy gate: unresolved imports and validation rules are hard failures.
    if int(cusa.get("unresolved_imports", 0)) > 0:
        failed += 1
    if rules:
        failed += 1
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
