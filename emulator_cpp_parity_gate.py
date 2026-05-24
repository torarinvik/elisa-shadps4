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
FALLBACK_WORKQUEUE_PATH = ROOT / "CUSA07399_FALLBACK_WORKQUEUE.md"
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
    steps = [
        Step("project.json syntax", [sys.executable, "-m", "json.tool", str(PROJECT_PATH)], category="validation"),
        Step("parity ledger", [sys.executable, "parity_ledger_check.py", "--summary"], category="ledger"),
        Step("parity ABI guard", [sys.executable, "parity_abi_check.py", "--summary"], category="ledger"),
        Step("emulator ABI smoke", [sys.executable, "emulator_abi_smoke.py"], category="validation"),
        Step(
            "strict native ABI contracts",
            [
                "go",
                "run",
                "./src",
                "project",
                "abi-lint",
                "emulator-cusa07399-x64-exec",
                "--project",
                "../elisa-shad-ps4-from-scratch",
                "-target-triple",
                target_triple_for_x64_host(),
                "--strict-contracts",
            ],
            cwd=COMPILER_DIR,
            category="guest-exec",
            timeout_seconds=120,
        ),
        Step("parity workqueue summary", [sys.executable, "parity_workqueue.py", "--fail-missing"], category="ledger"),
        Step("bridge syntax", [sys.executable, "check_elisa_bridges.py"], category="bridge"),
        compiler_test("core-libraries-audio-parity-tests", category="audio"),
        compiler_test("core-libraries-np-parity-tests", category="hle"),
        Step(
            "guest exec ABI smoke x64",
            [sys.executable, "emulator_guest_exec_abi_smoke.py", target_triple_for_x64_host()],
            category="guest-exec",
        ),
        Step(
            "CUSA07399 x64 real execution lane",
            [sys.executable, "emulator_cusa07399_x64_exec.py"],
            category="guest-exec",
            timeout_seconds=360,
        ),
        Step(
            "emulator ABI smoke x64",
            [
                sys.executable,
                "emulator_abi_smoke.py",
                target_triple_for_x64_host(),
                "--golden-dir",
                "abi_manifests",
                "--strict",
            ],
            category="guest-exec",
        ),
        Step(
            "native kernel_threads_runtime warnings",
            ["clang", "-Wall", "-Wextra", "-Werror", "-pthread", "-c", "native/kernel_threads_runtime.c", "-o", str(TMP_DIR / "kernel_threads_runtime.o")],
            category="native",
        ),
        compiler_test("emulator-core-boot-smoke", category="boot"),
        compiler_test("real-self-loader-tests", category="loader"),
        compiler_test("emulator-real-game-boot-smoke", category="cusa"),
        compiler_test("emulator-runtime-services-smoke", category="audio-input-system"),
        compiler_test("emulator-guest-exec-runtime-tests", category="guest-exec"),
        compiler_test("emulator-renderer-first-frame-smoke", category="graphics"),
        compiler_test("kernel-threads-runtime-tests", slow=True, category="threads"),
        compiler_test("video-core-renderer-vulkan-diagnostics-tests", slow=True, category="graphics"),
        compiler_test("video-core-multi-level-page-table-tests", slow=True, category="graphics"),
        compiler_test("core-libraries-avplayer", slow=True, category="audio-input-system"),
        compiler_test("core-libraries-pad-tests", slow=True, category="audio-input-system"),
        compiler_test_source("elisa_tests/core_libraries_save_data_parity_tests.elisa", category="save-dialog-misc"),
        compiler_test_source("elisa_tests/core_libraries_save_data_dialog_pure_tests.elisa", category="save-dialog-misc"),
        compiler_test_source("elisa_tests/core_libraries_web_browser_dialog_pure_tests.elisa", category="save-dialog-misc"),
        compiler_test_source("elisa_tests/core_libraries_signin_dialog_pure_tests.elisa", category="save-dialog-misc"),
        compiler_test_source("elisa_tests/core_libraries_playgo_pure_tests.elisa", category="save-dialog-misc"),
        compiler_test_source("elisa_tests/core_libraries_ime_dialog_parity_tests.elisa", category="save-dialog-misc"),
    ]
    if host_audio_backends_present():
        steps.append(compiler_test_source("elisa_tests/core_libraries_audioout_isolated.elisa", slow=True, category="audio"))
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
                row[token.strip()] = "1"
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
        if r.step.name in {
            "elisacore test emulator-real-game-boot-smoke",
            "elisacore test emulator-runtime-services-smoke",
        }:
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
        "user_service_initialized": 0,
        "user_service_initial_user": 0,
        "user_service_login_user_count": 0,
        "pad_initialized": 0,
        "pad_open_attempted": 0,
        "pad_opened": 0,
        "pad_no_controller": 0,
        "pad_read_attempted": 0,
        "pad_read_rc": 0,
        "pad_closed": 0,
        "audio_output_attempted": 0,
        "audio_output_opened": 0,
        "audio_output_write_attempted": 0,
        "audio_output_write_ok": 0,
        "audio_output_closed": 0,
        "audio_input_attempted": 0,
        "audio_input_opened": 0,
        "audio_input_read_attempted": 0,
        "audio_input_read_rc": 0,
        "audio_input_silent_state": 0,
        "audio_input_closed": 0,
        "relocation_count": 0,
        "execution_stage": 0,
        "boundary_status": 0,
        "guest_exec_boundary_reason_name": "",
        "guest_exec_host_arch": "",
        "guest_exec_host_mode": "",
        "guest_exec_supported_native_execution": 0,
        "guest_exec_x64_subprocess_available": 0,
        "guest_exec_probe_only": 0,
        "guest_exec_last_module": "",
        "guest_exec_last_symbol": "",
        "last_hle_module": "",
        "last_hle_symbol": "",
        "guest_exec_started": 0,
        "guest_exec_entry_reached": 0,
        "guest_exec_first_boundary_reached": 0,
        "guest_exec_boundary_reason": 0,
        "guest_exec_last_pc": 0,
        "guest_exec_last_sp": 0,
        "guest_exec_last_bp": 0,
        "guest_exec_last_rdi": 0,
        "guest_exec_expected_entry_params": 0,
        "guest_exec_expected_stack_word0": 0,
        "guest_exec_expected_stack_word1": 0,
        "guest_exec_entry_stack_word0": 0,
        "guest_exec_entry_stack_word1": 0,
        "guest_exec_fault_word0": 0,
        "guest_exec_fault_word1": 0,
        "guest_exec_signal_stack_word0": 0,
        "guest_exec_signal_stack_word1": 0,
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
    def set_last_int(key: str, value: str | None) -> None:
        if value is None:
            return
        parsed = to_int(value)
        if parsed != 0 or int(summary[key]) == 0:
            summary[key] = parsed
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
        summary["user_service_initialized"] = max(int(summary["user_service_initialized"]), to_int(row.get("user_service_initialized")))
        summary["user_service_initial_user"] = max(int(summary["user_service_initial_user"]), to_int(row.get("user_service_initial_user")))
        summary["user_service_login_user_count"] = max(int(summary["user_service_login_user_count"]), to_int(row.get("user_service_login_user_count")))
        summary["pad_initialized"] = max(int(summary["pad_initialized"]), to_int(row.get("pad_initialized")))
        summary["pad_open_attempted"] = max(int(summary["pad_open_attempted"]), to_int(row.get("pad_open_attempted")))
        summary["pad_opened"] = max(int(summary["pad_opened"]), to_int(row.get("pad_opened")))
        summary["pad_no_controller"] = max(int(summary["pad_no_controller"]), to_int(row.get("pad_no_controller")))
        summary["pad_read_attempted"] = max(int(summary["pad_read_attempted"]), to_int(row.get("pad_read_attempted")))
        set_last_int("pad_read_rc", row.get("pad_read_rc"))
        summary["pad_closed"] = max(int(summary["pad_closed"]), to_int(row.get("pad_closed")))
        summary["audio_output_attempted"] = max(int(summary["audio_output_attempted"]), to_int(row.get("audio_output_attempted")))
        summary["audio_output_opened"] = max(int(summary["audio_output_opened"]), to_int(row.get("audio_output_opened")))
        summary["audio_output_write_attempted"] = max(int(summary["audio_output_write_attempted"]), to_int(row.get("audio_output_write_attempted")))
        summary["audio_output_write_ok"] = max(int(summary["audio_output_write_ok"]), to_int(row.get("audio_output_write_ok")))
        summary["audio_output_closed"] = max(int(summary["audio_output_closed"]), to_int(row.get("audio_output_closed")))
        summary["audio_input_attempted"] = max(int(summary["audio_input_attempted"]), to_int(row.get("audio_input_attempted")))
        summary["audio_input_opened"] = max(int(summary["audio_input_opened"]), to_int(row.get("audio_input_opened")))
        summary["audio_input_read_attempted"] = max(int(summary["audio_input_read_attempted"]), to_int(row.get("audio_input_read_attempted")))
        set_last_int("audio_input_read_rc", row.get("audio_input_read_rc"))
        summary["audio_input_silent_state"] = max(int(summary["audio_input_silent_state"]), to_int(row.get("audio_input_silent_state")))
        summary["audio_input_closed"] = max(int(summary["audio_input_closed"]), to_int(row.get("audio_input_closed")))
        summary["relocation_count"] = max(int(summary["relocation_count"]), to_int(row.get("relocated_imports")))
        summary["execution_stage"] = max(int(summary["execution_stage"]), to_int(row.get("execution_stage")))
        set_last_int("boundary_status", row.get("boundary_status"))
        if row.get("guest_exec_boundary_reason_name"):
            summary["guest_exec_boundary_reason_name"] = row.get("guest_exec_boundary_reason_name", "")
        if row.get("guest_exec_host_arch"):
            summary["guest_exec_host_arch"] = row.get("guest_exec_host_arch", "")
        if row.get("guest_exec_host_mode"):
            summary["guest_exec_host_mode"] = row.get("guest_exec_host_mode", "")
        summary["guest_exec_supported_native_execution"] = max(int(summary["guest_exec_supported_native_execution"]), to_int(row.get("guest_exec_supported_native_execution")))
        summary["guest_exec_x64_subprocess_available"] = max(int(summary["guest_exec_x64_subprocess_available"]), to_int(row.get("guest_exec_x64_subprocess_available")))
        summary["guest_exec_probe_only"] = max(int(summary["guest_exec_probe_only"]), to_int(row.get("guest_exec_probe_only")))
        if row.get("guest_exec_last_module"):
            summary["guest_exec_last_module"] = row.get("guest_exec_last_module", "")
        if row.get("guest_exec_last_symbol"):
            summary["guest_exec_last_symbol"] = row.get("guest_exec_last_symbol", "")
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
        if row.get("guest_exec_last_sp") is not None:
            summary["guest_exec_last_sp"] = max(int(summary["guest_exec_last_sp"]), to_int(row.get("guest_exec_last_sp")))
        if row.get("guest_exec_last_bp") is not None:
            summary["guest_exec_last_bp"] = max(int(summary["guest_exec_last_bp"]), to_int(row.get("guest_exec_last_bp")))
        if row.get("guest_exec_last_rdi") is not None:
            summary["guest_exec_last_rdi"] = max(int(summary["guest_exec_last_rdi"]), to_int(row.get("guest_exec_last_rdi")))
        if row.get("guest_exec_expected_entry_params") is not None:
            summary["guest_exec_expected_entry_params"] = max(int(summary["guest_exec_expected_entry_params"]), to_int(row.get("guest_exec_expected_entry_params")))
        if row.get("guest_exec_expected_stack_word0") is not None:
            summary["guest_exec_expected_stack_word0"] = max(int(summary["guest_exec_expected_stack_word0"]), to_int(row.get("guest_exec_expected_stack_word0")))
        if row.get("guest_exec_expected_stack_word1") is not None:
            summary["guest_exec_expected_stack_word1"] = max(int(summary["guest_exec_expected_stack_word1"]), to_int(row.get("guest_exec_expected_stack_word1")))
        if row.get("guest_exec_entry_stack_word0") is not None:
            summary["guest_exec_entry_stack_word0"] = max(int(summary["guest_exec_entry_stack_word0"]), to_int(row.get("guest_exec_entry_stack_word0")))
        if row.get("guest_exec_entry_stack_word1") is not None:
            summary["guest_exec_entry_stack_word1"] = max(int(summary["guest_exec_entry_stack_word1"]), to_int(row.get("guest_exec_entry_stack_word1")))
        if row.get("guest_exec_fault_word0") is not None:
            summary["guest_exec_fault_word0"] = max(int(summary["guest_exec_fault_word0"]), to_int(row.get("guest_exec_fault_word0")))
        if row.get("guest_exec_fault_word1") is not None:
            summary["guest_exec_fault_word1"] = max(int(summary["guest_exec_fault_word1"]), to_int(row.get("guest_exec_fault_word1")))
        if row.get("guest_exec_signal_stack_word0") is not None:
            summary["guest_exec_signal_stack_word0"] = max(int(summary["guest_exec_signal_stack_word0"]), to_int(row.get("guest_exec_signal_stack_word0")))
        if row.get("guest_exec_signal_stack_word1") is not None:
            summary["guest_exec_signal_stack_word1"] = max(int(summary["guest_exec_signal_stack_word1"]), to_int(row.get("guest_exec_signal_stack_word1")))
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


def parse_prefixed_kv_line(line: str, prefix: str) -> dict[str, str]:
    text = line.strip()
    if not text.startswith(prefix):
        return {}
    payload = text.removeprefix(prefix).strip()
    row: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        row[key.strip()] = value.strip()
    return row


def parse_x64_exec_status(results: list[Result]) -> dict[str, str]:
    for r in results:
        if r.step.name != "CUSA07399 x64 real execution lane":
            continue
        last: dict[str, str] = {}
        for line in r.output.splitlines():
            row = parse_prefixed_kv_line(line, "CUSA07399_X64_EXEC")
            if row:
                last = row
        if last:
            last["step_returncode"] = str(r.returncode)
            return last
        return {"status": "missing-output", "step_returncode": str(r.returncode)}
    return {"status": "not-run", "step_returncode": "0"}


def subsystem_guess(library: str, module: str, nid: str) -> str:
    probe = f"{library} {module} {nid}".lower()
    if "ajm" in probe:
        return "audio_input_service"
    if "video" in probe or "gnm" in probe or "vulkan" in probe:
        return "graphics"
    if "font" in probe:
        return "graphics"
    if "audio" in probe:
        return "audio_input_service"
    if "savedata" in probe or "commondialog" in probe or "webbrowserdialog" in probe or "signindialog" in probe:
        return "save_dialog_misc"
    if "pad" in probe or "controller" in probe or "hmd" in probe or "ime" in probe:
        return "audio_input_service"
    if "kernel" in probe or "loader" in probe or "exec" in probe:
        return "loader_guest_exec"
    return "kernel"


def fallback_symbols(results: list[Result]) -> list[dict[str, str | int]]:
    rows = gather_artifact_rows(results)
    grouped: dict[tuple[str, str, str, str, str, str], dict[str, str | int]] = {}
    for row in rows:
        if row.get("resolution") != "3":
            continue
        if row.get("fallback_import") != "1":
            continue
        nid = row.get("nid", "")
        library = row.get("library", "")
        module = row.get("module", "")
        source = row.get("source", "")
        subsystem = row.get("subsystem", "")
        if subsystem == "":
            subsystem = subsystem_guess(library, module, nid)
        key = (nid, library, module, source, subsystem, "AeroLibFallback")
        if key not in grouped:
            grouped[key] = {
                "nid": nid,
                "library": library,
                "module": module,
                "source": source,
                "subsystem": subsystem,
                "status": "AeroLibFallback",
                "count": 0,
            }
        grouped[key]["count"] = int(grouped[key]["count"]) + 1
    rows2 = list(grouped.values())
    rows2.sort(key=lambda item: (-int(item["count"]), str(item["source"]), str(item["library"]), str(item["nid"])))
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


def boundary_probe_observed(cusa: dict[str, int | str]) -> bool:
    return (
        int(cusa.get("boundary_status", 0)) != 0
        or int(cusa.get("guest_exec_first_boundary_reached", 0)) != 0
        or int(cusa.get("guest_exec_boundary_reason", 0)) != 0
        or (
            int(cusa.get("guest_exec_probe_only", 0)) == 1
            and int(cusa.get("guest_exec_entry_reached", 0)) == 1
        )
    )


def cusa_stage_summary(results: list[Result], cusa: dict[str, int | str]) -> dict[str, str]:
    def passed(name: str) -> bool:
        return any(r.step.name == name and r.passed for r in results)

    load = "PASS" if passed("elisacore test emulator-real-game-boot-smoke") else "FAIL"
    link = "PASS" if load == "PASS" and int(cusa["unresolved_imports"]) == 0 else "FAIL"
    handoff = "PASS" if passed("elisacore test emulator-guest-exec-runtime-tests") else "FAIL"
    exec_stage = stage_name(int(cusa["execution_stage"]))
    boundary = "PASS" if boundary_probe_observed(cusa) else "PENDING"
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


def step_passed(results: list[Result], name: str) -> bool:
    return any(r.step.name == name and r.passed for r in results)


def host_exec_note() -> str:
    machine = platform.machine().lower()
    if sys.platform == "darwin" and machine in {"arm64", "aarch64"}:
        return "arm64 macOS probe-only"
    if sys.platform == "win32":
        return "Windows trampoline not implemented yet"
    if machine in {"x86_64", "amd64"}:
        return f"x86_64 host can attempt guarded guest execution on {platform.system()}"
    return f"probe-only on {platform.system()} {platform.machine()}"


def target_triple_for_x64_host() -> str:
    if sys.platform == "darwin":
        return "x86_64-apple-darwin"
    if sys.platform.startswith("linux"):
        return "x86_64-unknown-linux-gnu"
    if sys.platform == "win32":
        return "x86_64-pc-windows-msvc"
    return "x86_64-unknown-unknown"


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


def validate_rules(results: list[Result], cusa: dict[str, int | str], fallback_rows: list[dict[str, str | int]], require_first_boundary: bool, require_x64_real_exec: bool) -> list[str]:
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
        if boundary_status != 1 and first_boundary_reached == 0:
            errors.append("First-boundary probe requested but CUSA07399 did not reach the first guarded boundary")
    x64_exec = parse_x64_exec_status(results)
    if require_x64_real_exec and x64_exec.get("status") != "ok":
        errors.append(f"CUSA07399 x64 real execution requested but lane status is {x64_exec.get('status', 'unknown')}")

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
        "save_dialog_misc_fallbacks": [],
        "loader_guest_exec_blockers": [],
    }
    for row in rows:
        subsystem = str(row.get("subsystem", ""))
        if subsystem == "graphics":
            queues["graphics_fallbacks"].append(row)
        elif subsystem == "audio_input_service":
            queues["audio_input_service_fallbacks"].append(row)
        elif subsystem == "save_dialog_misc":
            queues["save_dialog_misc_fallbacks"].append(row)
        elif subsystem == "loader_guest_exec":
            queues["loader_guest_exec_blockers"].append(row)
        else:
            queues["kernel_fallbacks"].append(row)
    for key in queues:
        queues[key] = sorted(
            queues[key],
            key=lambda item: (-int(item.get("count", 0)), str(item.get("source", "")), str(item.get("library", "")), str(item.get("nid", ""))),
        )
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
    if boundary_probe_observed(cusa):
        passed += 1
    if not errors:
        passed += 1
    return passed, total


def summarize_progress(results: list[Result], require_first_boundary: bool = False, require_x64_real_exec: bool = False) -> tuple[str, dict[str, list[dict[str, str | int]]], dict[str, int | str], list[str], list[dict[str, str | int]]]:
    lines: list[str] = []
    ledger_counts = count_ledger_statuses()
    cusa = parse_cusa_metrics(results)
    fallback_rows = fallback_symbols(results)
    queues = build_fallback_queues(fallback_rows)
    rules = validate_rules(results, cusa, fallback_rows, require_first_boundary, require_x64_real_exec)
    stages = cusa_stage_summary(results, cusa)
    x64_exec = parse_x64_exec_status(results)

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
    synthetic_renderer = "PASS" if any(r.step.name == "elisacore test emulator-renderer-first-frame-smoke" and r.passed for r in results) else "FAIL"
    lines.append(f"- synthetic renderer smoke: {synthetic_renderer}")
    lines.append(f"- real CUSA first frame: {stages['frame']}")
    lines.append(f"- first frame ladder: {stages['frame_ladder']}")
    lines.append(f"- host note: {host_exec_note()}")
    lines.append(f"- guest exec host arch: {cusa['guest_exec_host_arch']}")
    lines.append(f"- guest exec host mode: {cusa['guest_exec_host_mode']}")
    lines.append(f"- guest exec supported native execution: {cusa['guest_exec_supported_native_execution']}")
    lines.append(f"- guest exec x64 subprocess available: {cusa['guest_exec_x64_subprocess_available']}")
    lines.append(f"- guest exec x64 subprocess smoke: {'PASS' if step_passed(results, 'guest exec x64 subprocess smoke') else 'FAIL'}")
    lines.append(f"- CUSA07399 x64 real execution lane: {x64_exec.get('status', 'not-run')}")
    if x64_exec.get("mode"):
        lines.append(f"- CUSA07399 x64 execution mode: {x64_exec.get('mode')}")
    if x64_exec.get("reason"):
        lines.append(f"- CUSA07399 x64 execution reason: {x64_exec.get('reason')}")
    if x64_exec.get("boundary_status"):
        lines.append(f"- CUSA07399 x64 boundary status: {x64_exec.get('boundary_status')}")
    if x64_exec.get("last_pc"):
        lines.append(f"- CUSA07399 x64 last pc: {x64_exec.get('last_pc')}")
    if x64_exec.get("last_sp"):
        lines.append(f"- CUSA07399 x64 last sp: {x64_exec.get('last_sp')}")
    if x64_exec.get("last_bp"):
        lines.append(f"- CUSA07399 x64 last bp: {x64_exec.get('last_bp')}")
    if x64_exec.get("last_rdi"):
        lines.append(f"- CUSA07399 x64 last rdi: {x64_exec.get('last_rdi')}")
    if x64_exec.get("expected_entry_params"):
        lines.append(f"- CUSA07399 x64 expected EntryParams: {x64_exec.get('expected_entry_params')}")
    if x64_exec.get("expected_stack_word0") or x64_exec.get("expected_stack_word1"):
        lines.append(f"- CUSA07399 x64 expected stack words: {x64_exec.get('expected_stack_word0', '0')}, {x64_exec.get('expected_stack_word1', '0')}")
    if x64_exec.get("entry_stack_word0") or x64_exec.get("entry_stack_word1"):
        lines.append(f"- CUSA07399 x64 entry stack words: {x64_exec.get('entry_stack_word0', '0')}, {x64_exec.get('entry_stack_word1', '0')}")
    if x64_exec.get("fault_word0") or x64_exec.get("fault_word1"):
        lines.append(f"- CUSA07399 x64 fault words: {x64_exec.get('fault_word0', '0')}, {x64_exec.get('fault_word1', '0')}")
    if x64_exec.get("signal_stack_word0") or x64_exec.get("signal_stack_word1"):
        lines.append(f"- CUSA07399 x64 signal stack words: {x64_exec.get('signal_stack_word0', '0')}, {x64_exec.get('signal_stack_word1', '0')}")
    if x64_exec.get("diagnostic"):
        lines.append(f"- CUSA07399 x64 diagnostic: {x64_exec.get('diagnostic')}")
    lines.append(f"- guest exec probe only: {cusa['guest_exec_probe_only']}")
    lines.append(f"- guest exec started: {cusa['guest_exec_started']}")
    lines.append(f"- guest exec entry reached: {cusa['guest_exec_entry_reached']}")
    lines.append(f"- guest exec first boundary reached: {cusa['guest_exec_first_boundary_reached']}")
    lines.append(f"- guest exec boundary reason: {cusa['guest_exec_boundary_reason']} ({cusa['guest_exec_boundary_reason_name']})")
    lines.append(f"- guest exec boundary reason name: {cusa['guest_exec_boundary_reason_name']}")
    lines.append(f"- guest exec last pc: 0x{int(cusa['guest_exec_last_pc']):x}")
    lines.append(f"- guest exec last sp: 0x{int(cusa['guest_exec_last_sp']):x}")
    lines.append(f"- guest exec last bp: 0x{int(cusa['guest_exec_last_bp']):x}")
    lines.append(f"- guest exec last rdi: 0x{int(cusa['guest_exec_last_rdi']):x}")
    lines.append(f"- guest exec expected EntryParams: 0x{int(cusa['guest_exec_expected_entry_params']):x}")
    lines.append(f"- guest exec expected stack words: 0x{int(cusa['guest_exec_expected_stack_word0']):x}, 0x{int(cusa['guest_exec_expected_stack_word1']):x}")
    lines.append(f"- guest exec entry stack words: 0x{int(cusa['guest_exec_entry_stack_word0']):x}, 0x{int(cusa['guest_exec_entry_stack_word1']):x}")
    lines.append(f"- guest exec fault words: 0x{int(cusa['guest_exec_fault_word0']):x}, 0x{int(cusa['guest_exec_fault_word1']):x}")
    lines.append(f"- guest exec signal stack words: 0x{int(cusa['guest_exec_signal_stack_word0']):x}, 0x{int(cusa['guest_exec_signal_stack_word1']):x}")
    lines.append(f"- guest exec last signal: {cusa['guest_exec_last_signal']}")
    lines.append(f"- guest exec last module: {cusa['guest_exec_last_module']}")
    lines.append(f"- guest exec last symbol: {cusa['guest_exec_last_symbol']}")
    if int(cusa["guest_exec_probe_only"]) == 1:
        lines.append(f"- first boundary blocker: probe-only host ({cusa['guest_exec_host_arch']} {cusa['guest_exec_host_mode']})")
    elif int(cusa["guest_exec_supported_native_execution"]) == 0:
        lines.append(f"- first boundary blocker: unsupported host ({cusa['guest_exec_host_arch']} {cusa['guest_exec_host_mode']})")
    elif int(cusa["guest_exec_expected_entry_params"]) != 0 and int(cusa["guest_exec_last_rdi"]) != int(cusa["guest_exec_expected_entry_params"]):
        lines.append("- first boundary blocker: guest-entry-rdi-not-entryparams")
    elif (
        int(cusa["guest_exec_expected_stack_word0"]) != int(cusa["guest_exec_entry_stack_word0"])
        or int(cusa["guest_exec_expected_stack_word1"]) != int(cusa["guest_exec_entry_stack_word1"])
    ):
        lines.append("- first boundary blocker: guest-entry-stack-copy-mismatch")
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
    lines.append("Real CUSA runtime service signals:")
    lines.append(f"- real CUSA user service initialized: {cusa['user_service_initialized']}")
    lines.append(f"- user service initial user: {cusa['user_service_initial_user']}")
    lines.append(f"- user service login user count: {cusa['user_service_login_user_count']}")
    lines.append(f"- real CUSA pad initialized: {cusa['pad_initialized']}")
    lines.append(f"- pad open attempted/opened: {cusa['pad_open_attempted']}/{cusa['pad_opened']}")
    lines.append(f"- pad no controller: {cusa['pad_no_controller']}")
    lines.append(f"- pad read attempted/rc: {cusa['pad_read_attempted']}/{cusa['pad_read_rc']}")
    lines.append(f"- pad closed: {cusa['pad_closed']}")
    lines.append(f"- real CUSA audio output attempted/opened: {cusa['audio_output_attempted']}/{cusa['audio_output_opened']}")
    lines.append(f"- audio output write attempted/ok: {cusa['audio_output_write_attempted']}/{cusa['audio_output_write_ok']}")
    lines.append(f"- audio output closed: {cusa['audio_output_closed']}")
    lines.append(f"- real CUSA audio input attempted/opened: {cusa['audio_input_attempted']}/{cusa['audio_input_opened']}")
    lines.append(f"- audio input read attempted/rc: {cusa['audio_input_read_attempted']}/{cusa['audio_input_read_rc']}")
    lines.append(f"- audio input silent state: {cusa['audio_input_silent_state']}")
    lines.append(f"- audio input closed: {cusa['audio_input_closed']}")
    audio_skip_reason = parse_audio_skip_reason(results)
    if audio_skip_reason:
        lines.append(f"- audio runtime skip reason: {audio_skip_reason}")
    lines.append(f"- current execution stage: {cusa['execution_stage']} ({stage_name(int(cusa['execution_stage']))})")
    lines.append(f"- last HLE call: module={cusa['last_hle_module']} symbol={cusa['last_hle_symbol']}")
    lines.append(f"- current video/audio/input stage: graphics={len(queues['graphics_fallbacks'])} audio-input-service={len(queues['audio_input_service_fallbacks'])}")
    lines.append(f"- current save/dialog/misc fallback stage: save-dialog-misc={len(queues['save_dialog_misc_fallbacks'])}")
    lines.append("Save/Dialog/Misc parity test signals:")
    lines.append(f"- save data parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_save_data_parity_tests.elisa') else 'FAIL'}")
    lines.append(f"- save data dialog parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_save_data_dialog_pure_tests.elisa') else 'FAIL'}")
    lines.append(f"- web browser dialog parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_web_browser_dialog_pure_tests.elisa') else 'FAIL'}")
    lines.append(f"- signin dialog parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_signin_dialog_pure_tests.elisa') else 'FAIL'}")
    lines.append(f"- playgo parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_playgo_pure_tests.elisa') else 'FAIL'}")
    lines.append(f"- ime dialog parity tests: {'PASS' if step_passed(results, 'elisacore run elisa_tests/core_libraries_ime_dialog_parity_tests.elisa') else 'FAIL'}")

    lines.append("Top 50 fallback symbols:")
    for item in fallback_rows[:50]:
        lines.append(
            f"- [{item['count']}] nid={item['nid']} lib={item['library']} module={item['module']} source={item['source']} subsystem={item['subsystem']} status={item['status']}"
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

    return "\n".join(lines), queues, cusa, rules, fallback_rows


def parse_previous_fallback_count(previous_report: str | None) -> int | None:
    if not previous_report:
        return None
    match = re.search(r"^- AeroLib fallback count: (\d+)$", previous_report, re.MULTILINE)
    if not match:
        return None
    return int(match.group(1))


def write_report(
    path: Path,
    results: list[Result],
    progress: str,
    queues: dict[str, list[dict[str, str | int]]],
    fallback_before: int | None,
    fallback_after: int,
) -> None:
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    lines = ["# Emulator C++ Parity Gate", "", f"Passed: {passed}", f"Failed: {failed}", "", progress, "", "## Agent Work Queues"]
    for qname, items in queues.items():
        lines.append(f"### {qname}")
        if not items:
            lines.append("- none")
            continue
        for item in items[:25]:
            lines.append(
                f"- [{item['count']}] {item['nid']} {item['library']} {item['module']} ({item['source']}) subsystem={item['subsystem']} status={item.get('status', 'AeroLibFallback')}"
            )
    if fallback_before is not None:
        lines.extend(
            [
                "",
                "## Fallback Delta",
                f"- fallback count before: {fallback_before}",
                f"- fallback count after: {fallback_after}",
                f"- fallback symbols newly resolved: {max(fallback_before - fallback_after, 0)}",
            ]
        )
    lines.extend(["", "## Steps"])
    for r in results:
        status = "PASS" if r.passed else "FAIL"
        lines.append(f"- {status}: {r.step.name}")
    path.write_text("\n".join(lines) + "\n")


def write_fallback_workqueue(
    path: Path,
    fallback_rows: list[dict[str, str | int]],
    cusa: dict[str, int | str],
    fallback_before: int | None,
) -> None:
    lines = [
        "# CUSA07399 Fallback Workqueue",
        "",
        f"Current AeroLib fallback count: {cusa['aerolib_fallback_imports']}",
    ]
    if fallback_before is not None:
        lines.append(f"Previous AeroLib fallback count: {fallback_before}")
        lines.append(f"Resolved since last report: {max(fallback_before - int(cusa['aerolib_fallback_imports']), 0)}")
    lines.extend(["", "## Top Fallback Symbols", "", "| Count | Status | Subsystem | NID | Library | Module | Source |", "|---|---|---|---|---|---|---|"])
    for item in fallback_rows[:50]:
        lines.append(
            f"| {item['count']} | {item.get('status', 'AeroLibFallback')} | {item['subsystem']} | `{item['nid']}` | `{item['library']}` | `{item['module']}` | `{item['source']}` |"
        )
    lines.extend(["", "## Full Queue", "", "| Count | Status | Subsystem | NID | Library | Module | Source |", "|---|---|---|---|---|---|---|"])
    for item in fallback_rows:
        lines.append(
            f"| {item['count']} | {item.get('status', 'AeroLibFallback')} | {item['subsystem']} | `{item['nid']}` | `{item['library']}` | `{item['module']}` | `{item['source']}` |"
        )
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
    parser.add_argument("--require-x64-real-exec", action="store_true", help="Fail unless CUSA07399 runs in an x86_64 Elisa emulator process past handoff.")
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

    progress, queues, cusa, rules, fallback_rows = summarize_progress(results, args.require_first_boundary, args.require_x64_real_exec)
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed

    print("\n== Parity Progress ==")
    print(progress)
    print(f"\nGate result: passed={passed} failed={failed} selected={len(results)}")

    previous_report = LAST_REPORT_PATH.read_text() if LAST_REPORT_PATH.exists() else None
    fallback_before = parse_previous_fallback_count(previous_report)
    report_path = args.write_report if args.write_report else LAST_REPORT_PATH
    write_report(report_path, results, progress, queues, fallback_before, int(cusa["aerolib_fallback_imports"]))
    print(f"wrote report: {report_path}")
    current_report = report_path.read_text()
    if report_path != LAST_REPORT_PATH:
        LAST_REPORT_PATH.write_text(current_report)
    write_fallback_workqueue(FALLBACK_WORKQUEUE_PATH, fallback_rows, cusa, fallback_before)
    print(f"wrote fallback queue: {FALLBACK_WORKQUEUE_PATH}")

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
