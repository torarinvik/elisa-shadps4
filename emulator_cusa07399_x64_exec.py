#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"
ARTIFACT_PATH = ROOT / "cusa07399_artifacts.txt"
HANDOFF_MANIFEST_PATH = ROOT / "cusa07399_handoff_manifest.txt"
CUSA_PATH = ROOT.parent / "shadPS4" / "Games" / "CUSA07399"


def run(cmd: list[str], cwd: Path = ROOT, timeout: int = 120) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
    )


def emit(status: str, **fields: object) -> None:
    payload = [f"status={status}"]
    for key, value in fields.items():
        text = str(value).replace(" ", "_")
        payload.append(f"{key}={text}")
    print("CUSA07399_X64_EXEC " + " ".join(payload))


def parse_kv_line(line: str) -> dict[str, str]:
    row: dict[str, str] = {}
    for token in line.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        row[key.strip()] = value.strip()
    return row


def parse_artifacts() -> dict[str, str]:
    summary: dict[str, str] = {}
    if ARTIFACT_PATH.exists():
        for line in ARTIFACT_PATH.read_text().splitlines():
            if "CUSA07399_ARTIFACT" not in line:
                continue
            summary.update(parse_kv_line(line.split("CUSA07399_ARTIFACT", 1)[1].strip()))
    if HANDOFF_MANIFEST_PATH.exists():
        for line in HANDOFF_MANIFEST_PATH.read_text().splitlines():
            if line.startswith("CUSA07399_HANDOFF_MANIFEST"):
                for key, value in parse_kv_line(line).items():
                    summary[f"handoff_{key}"] = value
                break
    return summary


def artifact_get(artifacts: dict[str, str], *keys: str, default: str = "0") -> str:
    for key in keys:
        value = artifacts.get(key)
        if value not in (None, ""):
            return value
    return default


def to_int(value: str | None) -> int:
    if value is None or value == "":
        return 0
    try:
        return int(value, 16) if value.lower().startswith("0x") else int(value)
    except ValueError:
        return 0


def rosetta_available() -> bool:
    arch = Path("/usr/bin/arch")
    uname = Path("/usr/bin/uname")
    if not arch.exists() or not uname.exists():
        return False
    try:
        proc = run([str(arch), "-x86_64", str(uname), "-m"], timeout=10)
    except Exception:
        return False
    return proc.returncode == 0 and proc.stdout.strip() == "x86_64"


def select_mode() -> tuple[bool, str]:
    machine = platform.machine().lower()
    if machine in {"x86_64", "amd64"}:
        return True, "native-x86_64"
    if sys.platform == "darwin" and machine in {"arm64", "aarch64"}:
        if rosetta_available():
            return True, "cross-emit-rosetta-x86_64"
        return False, "rosetta-unavailable"
    return False, f"unsupported-host-{platform.system()}-{platform.machine()}"


def go_available() -> tuple[bool, str, str]:
    go = shutil.which("go")
    if go is None:
        return False, "go-missing", ""
    try:
        proc = run([go, "env", "GOARCH"], cwd=COMPILER_DIR, timeout=20)
    except Exception as exc:
        return False, "go-env-failed", str(exc)
    arch = proc.stdout.strip().splitlines()[-1] if proc.stdout.strip() else ""
    if proc.returncode != 0:
        return False, "go-env-failed", proc.stdout.strip()
    return True, go, arch


def target_triple_for_host() -> str:
    if sys.platform == "darwin":
        return "x86_64-apple-darwin"
    if sys.platform.startswith("linux"):
        return "x86_64-unknown-linux-gnu"
    if sys.platform == "win32":
        return "x86_64-pc-windows-msvc"
    return "x86_64-unknown-unknown"


def validate_artifacts(artifacts: dict[str, str]) -> list[str]:
    errors: list[str] = []
    if to_int(artifacts.get("unresolved_imports")) != 0:
        errors.append("unresolved-imports")
    if to_int(artifacts.get("aerolib_fallback")) != 0:
        errors.append("aerolib-fallbacks")
    if to_int(artifacts.get("guest_exec_supported_native_execution")) != 1:
        errors.append("unsupported-native-exec")
    if to_int(artifacts.get("guest_exec_probe_only")) != 0:
        errors.append("probe-only")
    if to_int(artifacts.get("entry")) == 0 or to_int(artifacts.get("main_entry")) == 0:
        errors.append("missing-entry")
    if to_int(artifacts.get("handoff_entry_word0")) == 0 and to_int(artifacts.get("handoff_entry_word1")) == 0:
        errors.append("zero-entry-bytes")
    if to_int(artifacts.get("handoff_entry_in_exec")) != 1:
        errors.append("entry-outside-exec-segment")
    boundary_status = to_int(artifact_get(artifacts, "boundary_status"))
    execution_stage = to_int(artifact_get(artifacts, "execution_stage"))
    last_pc = to_int(artifact_get(artifacts, "guest_exec_last_pc", "last_guest_pc"))
    last_sp = to_int(artifact_get(artifacts, "guest_exec_last_sp", "last_guest_sp"))
    last_signal = to_int(artifact_get(artifacts, "guest_exec_last_signal", "last_signal"))
    accepted_boundary = boundary_status in {1, 2, 3, 4}
    accepted_captured_fault = boundary_status == -10 and execution_stage >= 6 and last_signal != 0
    accepted_timeout = boundary_status == -3 and execution_stage >= 4
    if not (accepted_boundary or accepted_captured_fault or accepted_timeout):
        errors.append(f"bad-boundary-status-{boundary_status}")
    if boundary_status == -10 and last_pc == 0:
        errors.append("missing-crash-pc")
    if boundary_status == -10 and last_sp == 0:
        errors.append("missing-crash-sp")
    if execution_stage < 4:
        errors.append("execution-stage-before-guarded-entry")
    return errors


def diagnose_artifacts(artifacts: dict[str, str]) -> str:
    notes: list[str] = []
    boundary_status = to_int(artifact_get(artifacts, "boundary_status"))
    main_entry = to_int(artifact_get(artifacts, "main_entry"))
    last_pc = to_int(artifact_get(artifacts, "guest_exec_last_pc", "last_guest_pc"))
    last_rdi = to_int(artifact_get(artifacts, "guest_exec_last_rdi", "last_guest_rdi"))
    expected_params = to_int(artifact_get(artifacts, "guest_exec_expected_entry_params"))
    expected_stack0 = to_int(artifact_get(artifacts, "guest_exec_expected_stack_word0"))
    expected_stack1 = to_int(artifact_get(artifacts, "guest_exec_expected_stack_word1"))
    entry_stack0 = to_int(artifact_get(artifacts, "guest_exec_entry_stack_word0"))
    entry_stack1 = to_int(artifact_get(artifacts, "guest_exec_entry_stack_word1"))
    handoff_word0 = to_int(artifact_get(artifacts, "handoff_entry_word0"))
    handoff_word1 = to_int(artifact_get(artifacts, "handoff_entry_word1"))
    parent_native_word0 = to_int(artifact_get(artifacts, "guest_exec_main_entry_native_word0"))
    parent_native_word1 = to_int(artifact_get(artifacts, "guest_exec_main_entry_native_word1"))
    child_native_word0 = to_int(artifact_get(artifacts, "guest_exec_entry_native_word0"))
    child_native_word1 = to_int(artifact_get(artifacts, "guest_exec_entry_native_word1"))
    pc_native_prot = to_int(artifact_get(artifacts, "guest_exec_last_pc_native_prot"))
    argc = to_int(artifact_get(artifacts, "guest_exec_last_argc", "last_argc"))
    if boundary_status == -10 and expected_params != 0 and last_rdi != expected_params:
        notes.append("guest-entry-rdi-not-entryparams")
    if boundary_status == -10 and (
        expected_stack0 != entry_stack0 or expected_stack1 != entry_stack1
    ):
        notes.append("guest-entry-stack-copy-mismatch")
    if boundary_status == -10 and main_entry != 0 and last_pc == main_entry and argc != 0 and last_rdi == argc:
        notes.append("entry-rdi-equals-argc")
    elif boundary_status == -10 and main_entry != 0 and last_pc == main_entry and 0 < last_rdi <= 64:
        notes.append("entry-rdi-is-small-integer")
    if boundary_status == -10 and handoff_word0 != 0 and parent_native_word0 == 0:
        notes.append("parent-native-entry-zero")
    if boundary_status == -10 and parent_native_word0 != 0 and child_native_word0 == 0:
        notes.append("child-native-entry-zero")
    if boundary_status == -10 and parent_native_word0 != 0 and (
        parent_native_word0 != child_native_word0 or parent_native_word1 != child_native_word1
    ):
        notes.append("native-entry-parent-child-mismatch")
    if boundary_status == -10 and pc_native_prot == 0xFFFFFFFF:
        notes.append("last-pc-not-in-native-region")
    elif boundary_status == -10 and (pc_native_prot & 1) == 0:
        notes.append("last-pc-not-readable")
    if boundary_status == -10 and artifact_get(artifacts, "last_hle_symbol", default="") != "":
        notes.append("last-hle-recorded-before-crash")
    return ",".join(notes) if notes else "none"


def run_abi_lint(go: str, mode: str, strict: bool, verbose: bool) -> bool:
    cmd = [
        go,
        "run",
        "./src",
        "project",
        "abi-lint",
        "emulator-cusa07399-x64-exec",
        "--project",
        "../elisa-shad-ps4-from-scratch",
        "-target-triple",
        target_triple_for_host(),
        "--strict-contracts",
    ]
    proc = run(cmd, cwd=COMPILER_DIR, timeout=120)
    if verbose or proc.returncode != 0:
        print(proc.stdout, end="")
    if proc.returncode == 0:
        return True
    emit("failed", mode=mode, reason="abi-lint-failed", returncode=proc.returncode)
    return not strict


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the real CUSA07399 guest-entry gate in an x86_64 Elisa emulator process when possible.")
    parser.add_argument("--strict", action="store_true", help="Fail when the x86_64 lane is unavailable or does not pass.")
    parser.add_argument("--verbose", action="store_true", help="Print the underlying test output.")
    args = parser.parse_args()

    if not CUSA_PATH.exists():
        emit("skipped", reason="cusa07399-missing")
        return 1 if args.strict else 0
    if not COMPILER_DIR.exists():
        emit("skipped", reason="compiler-missing")
        return 1 if args.strict else 0

    supported, mode = select_mode()
    if not supported:
        emit("skipped", reason=mode)
        return 1 if args.strict else 0

    ok, go_or_reason, detail = go_available()
    if not ok:
        emit("toolchain-unavailable", mode=mode, reason=go_or_reason, detail=detail or "none")
        return 1 if args.strict else 0

    if not run_abi_lint(go_or_reason, mode, args.strict, args.verbose):
        return 1

    cmd = [
        go_or_reason,
        "run",
        "./src",
        "test",
        "emulator-cusa07399-x64-exec",
        "--project",
        "../elisa-shad-ps4-from-scratch",
        "-target-triple",
        target_triple_for_host(),
    ]
    proc = run(cmd, cwd=COMPILER_DIR, timeout=300)
    if args.verbose or proc.returncode != 0:
        print(proc.stdout, end="")
    if proc.returncode != 0:
        emit("failed", mode=mode, reason="target-failed", returncode=proc.returncode)
        return 1

    artifacts = parse_artifacts()
    errors = validate_artifacts(artifacts)
    if errors:
        emit(
            "failed",
            mode=mode,
            reason=",".join(errors),
            guarded_status=artifact_get(artifacts, "guarded_status"),
            boundary_status=artifact_get(artifacts, "boundary_status"),
            boundary_reason=artifact_get(artifacts, "guest_exec_boundary_reason_name", "boundary_reason", default="unknown"),
            execution_stage=artifact_get(artifacts, "execution_stage"),
            native_phase=artifact_get(artifacts, "guest_exec_native_phase"),
            last_pc=artifact_get(artifacts, "guest_exec_last_pc", "last_guest_pc"),
            last_sp=artifact_get(artifacts, "guest_exec_last_sp", "last_guest_sp"),
            last_bp=artifact_get(artifacts, "guest_exec_last_bp", "last_guest_bp"),
            last_rdi=artifact_get(artifacts, "guest_exec_last_rdi", "last_guest_rdi"),
            last_rsi=artifact_get(artifacts, "guest_exec_last_rsi", "last_guest_rsi"),
            last_rax=artifact_get(artifacts, "guest_exec_last_rax", "last_guest_rax"),
            last_rbx=artifact_get(artifacts, "guest_exec_last_rbx", "last_guest_rbx"),
            expected_entry_params=artifact_get(artifacts, "guest_exec_expected_entry_params"),
            expected_stack_word0=artifact_get(artifacts, "guest_exec_expected_stack_word0"),
            expected_stack_word1=artifact_get(artifacts, "guest_exec_expected_stack_word1"),
            entry_stack_word0=artifact_get(artifacts, "guest_exec_entry_stack_word0"),
            entry_stack_word1=artifact_get(artifacts, "guest_exec_entry_stack_word1"),
            native_region_count=artifact_get(artifacts, "guest_exec_native_region_count"),
            main_entry_native_prot=artifact_get(artifacts, "guest_exec_main_entry_native_prot"),
            main_entry_native_word0=artifact_get(artifacts, "guest_exec_main_entry_native_word0"),
            main_entry_native_word1=artifact_get(artifacts, "guest_exec_main_entry_native_word1"),
            entry_native_word0=artifact_get(artifacts, "guest_exec_entry_native_word0"),
            entry_native_word1=artifact_get(artifacts, "guest_exec_entry_native_word1"),
            entry_native_prot=artifact_get(artifacts, "guest_exec_entry_native_prot"),
            last_pc_native_prot=artifact_get(artifacts, "guest_exec_last_pc_native_prot"),
            fault_word0=artifact_get(artifacts, "guest_exec_fault_word0"),
            fault_word1=artifact_get(artifacts, "guest_exec_fault_word1"),
            signal_stack_word0=artifact_get(artifacts, "guest_exec_signal_stack_word0"),
            signal_stack_word1=artifact_get(artifacts, "guest_exec_signal_stack_word1"),
            fault=artifact_get(artifacts, "guest_exec_fault_address", "fault"),
            last_signal=artifact_get(artifacts, "guest_exec_last_signal", "last_signal"),
            last_hle_module=artifact_get(artifacts, "last_hle_module", default=""),
            last_hle_symbol=artifact_get(artifacts, "last_hle_symbol", default=""),
            diagnostic=diagnose_artifacts(artifacts),
        )
        return 1

    emit(
        "ok",
        mode=mode,
        guarded_status=artifact_get(artifacts, "guarded_status"),
        boundary_status=artifact_get(artifacts, "boundary_status"),
        boundary_reason=artifact_get(artifacts, "guest_exec_boundary_reason_name", "boundary_reason", default="unknown"),
        execution_stage=artifact_get(artifacts, "execution_stage"),
        native_phase=artifact_get(artifacts, "guest_exec_native_phase"),
        last_pc=artifact_get(artifacts, "guest_exec_last_pc", "last_guest_pc"),
        last_sp=artifact_get(artifacts, "guest_exec_last_sp", "last_guest_sp"),
        last_bp=artifact_get(artifacts, "guest_exec_last_bp", "last_guest_bp"),
        last_rdi=artifact_get(artifacts, "guest_exec_last_rdi", "last_guest_rdi"),
        last_rsi=artifact_get(artifacts, "guest_exec_last_rsi", "last_guest_rsi"),
        last_rax=artifact_get(artifacts, "guest_exec_last_rax", "last_guest_rax"),
        last_rbx=artifact_get(artifacts, "guest_exec_last_rbx", "last_guest_rbx"),
        expected_entry_params=artifact_get(artifacts, "guest_exec_expected_entry_params"),
        expected_stack_word0=artifact_get(artifacts, "guest_exec_expected_stack_word0"),
        expected_stack_word1=artifact_get(artifacts, "guest_exec_expected_stack_word1"),
        entry_stack_word0=artifact_get(artifacts, "guest_exec_entry_stack_word0"),
        entry_stack_word1=artifact_get(artifacts, "guest_exec_entry_stack_word1"),
        native_region_count=artifact_get(artifacts, "guest_exec_native_region_count"),
        main_entry_native_prot=artifact_get(artifacts, "guest_exec_main_entry_native_prot"),
        main_entry_native_word0=artifact_get(artifacts, "guest_exec_main_entry_native_word0"),
        main_entry_native_word1=artifact_get(artifacts, "guest_exec_main_entry_native_word1"),
        entry_native_word0=artifact_get(artifacts, "guest_exec_entry_native_word0"),
        entry_native_word1=artifact_get(artifacts, "guest_exec_entry_native_word1"),
        entry_native_prot=artifact_get(artifacts, "guest_exec_entry_native_prot"),
        last_pc_native_prot=artifact_get(artifacts, "guest_exec_last_pc_native_prot"),
        fault_word0=artifact_get(artifacts, "guest_exec_fault_word0"),
        fault_word1=artifact_get(artifacts, "guest_exec_fault_word1"),
        signal_stack_word0=artifact_get(artifacts, "guest_exec_signal_stack_word0"),
        signal_stack_word1=artifact_get(artifacts, "guest_exec_signal_stack_word1"),
        fault=artifact_get(artifacts, "guest_exec_fault_address", "fault"),
        last_signal=artifact_get(artifacts, "guest_exec_last_signal", "last_signal"),
        last_hle_module=artifact_get(artifacts, "last_hle_module", default=""),
        last_hle_symbol=artifact_get(artifacts, "last_hle_symbol", default=""),
        diagnostic=diagnose_artifacts(artifacts),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
