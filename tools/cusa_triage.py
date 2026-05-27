#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import struct
from dataclasses import dataclass
from pathlib import Path

EVENT_SIZE = 48

EVENT_NAMES = {
    1: "GUARD_BEGIN",
    2: "GUEST_ENTER",
    3: "HLE_ENTER",
    4: "HLE_VALIDATE_BEGIN",
    5: "HLE_VALIDATE_END",
    6: "HLE_NATIVE_CALL_BEGIN",
    7: "HLE_NATIVE_CALL_END",
    8: "HLE_RETURN_BEGIN",
    9: "HLE_RETURN_END",
    10: "ALARM_FIRED",
    11: "SIGNAL_FIRED",
    12: "EXIT",
    13: "SIGNAL_ENTRY_RAW",
    14: "SIGNAL_AFTER_RESTORE",
    15: "SIGNAL_AFTER_SHADOW",
    16: "SIGNAL_AFTER_ASSERT",
    17: "SIGNAL_STACK_WORDS",
    18: "SIGNAL_RETURN_CALLSITE",
    19: "SIGNAL_CALL_TARGET",
    20: "MAIN_RUN_BEGIN",
    21: "MAIN_RUN_END",
    22: "TEST_AFTER_GUARD",
    110: "SSE4A_SIGILL_ENTER",
    111: "SSE4A_EXTRQ_HANDLED",
    112: "SSE4A_INSERTQ_HANDLED",
    113: "SSE4A_UNHANDLED",
    120: "ORACLE_BOUNDARY_CONT",
    121: "ORACLE_BOUNDARY_STACK",
    122: "ORACLE_BOUNDARY_META",
    1001: "MUTEX_INIT",
}

BOUNDARY_GUARD_REASONS = {
    0: "none",
    1: "poison",
    2: "near-null-unmapped",
    3: "non-canonical",
    4: "unmapped-exec",
    5: "unmapped-pointer",
    6: "unpublished-exec",
}


@dataclass(frozen=True)
class Event:
    seq: int
    slot: int
    kind: int
    phase: int
    a: int
    b: int
    c: int
    d: int

    @property
    def name(self) -> str:
        return EVENT_NAMES.get(self.kind, str(self.kind))


def parse_int(text: str) -> int | None:
    try:
        if text.startswith("-"):
            return int(text, 10)
        return int(text, 0)
    except ValueError:
        return None


def parse_artifacts(path: Path) -> dict[str, str]:
    facts: dict[str, str] = {}
    if not path.exists():
        return facts
    for line in path.read_text(errors="replace").splitlines():
        if not line.startswith("CUSA07399_ARTIFACT "):
            continue
        payload = line.split(" ", 1)[1]
        for key, value in re.findall(r"([A-Za-z0-9_]+)=([^ \t]+)", payload):
            facts[key] = value
    return facts


def parse_events(path: Path) -> list[Event]:
    if not path.exists():
        return []
    data = path.read_bytes()
    events: list[Event] = []
    for slot in range(min(256, len(data) // EVENT_SIZE)):
        seq, kind, phase, a, b, c, d = struct.unpack_from("<QI I QQQQ", data, slot * EVENT_SIZE)
        if seq:
            events.append(Event(seq, slot, kind, phase, a, b, c, d))
    events.sort(key=lambda event: event.seq)
    return events


def signed64(value: int) -> int:
    value &= (1 << 64) - 1
    if value & (1 << 63):
        return value - (1 << 64)
    return value


def fact_int(facts: dict[str, str], key: str, default: int = 0) -> int:
    value = facts.get(key)
    if value is None:
        return default
    parsed = parse_int(value)
    return default if parsed is None else parsed


def latest(events: list[Event], kind: int) -> Event | None:
    for event in reversed(events):
        if event.kind == kind:
            return event
    return None


def print_event(event: Event) -> None:
    print(
        f"  #{event.seq:04d} {event.name:<24} phase={event.phase:<4} "
        f"a=0x{event.a:x} b=0x{event.b:x} c=0x{event.c:x} d=0x{event.d:x}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Classify the latest CUSA07399 guest-exec run and print the next debugging move."
    )
    parser.add_argument("--artifact", default="cusa07399_artifacts.txt")
    parser.add_argument("--gewd", default="/tmp/elisa_gewd.bin")
    parser.add_argument("--tail", type=int, default=8)
    args = parser.parse_args()

    facts = parse_artifacts(Path(args.artifact))
    events = parse_events(Path(args.gewd))

    if not facts and not events:
        print("No CUSA artifact or GEWD tape found.")
        print("Run: ELISA_GUEST_EXEC_DEBUG_TRACE=1 ELISA_GUEST_EXEC_DEBUG_TRAP=1 python3 emulator_cusa07399_x64_exec.py")
        return 2

    guarded_status = fact_int(facts, "guarded_status")
    last_signal = fact_int(facts, "last_signal")
    first_boundary = fact_int(facts, "guest_exec_first_boundary_reached")
    runtime_ret = fact_int(facts, "guest_exec_runtime_hle_guest_return_addr")
    runtime_module = facts.get("guest_exec_runtime_hle_module", "")
    runtime_symbol = facts.get("guest_exec_runtime_hle_symbol", "")
    native_prot = fact_int(facts, "guest_exec_main_entry_native_prot", 0xFFFFFFFF)
    entry = fact_int(facts, "main_entry_addr", fact_int(facts, "main_entry"))
    boundary_reason_name = facts.get("guest_exec_boundary_reason_name", "")

    test_after = latest(events, 22)
    oracle_cont = latest(events, 120)
    oracle_stack = latest(events, 121)
    oracle_meta = latest(events, 122)
    signal_event = latest(events, 11) or latest(events, 13)

    print("CUSA triage")
    print("===========")
    print(f"entry=0x{entry:x} entry_native_prot=0x{native_prot:x}")
    print(
        f"guarded_status={guarded_status} last_signal={last_signal} "
        f"first_boundary={first_boundary} reason={boundary_reason_name or '?'}"
    )
    if runtime_symbol:
        print(f"first_hle={runtime_module}:{runtime_symbol} return=0x{runtime_ret:x}")
    if test_after:
        print(
            "test_after_guard="
            f"status={signed64(test_after.a)} signal={test_after.b} fs_owner={test_after.c} last_status={signed64(test_after.d)}"
        )
    if oracle_cont:
        reason = oracle_meta.a if oracle_meta else 0
        reason_text = BOUNDARY_GUARD_REASONS.get(reason, str(reason))
        print(
            "oracle_boundary="
            f"continuation=0x{oracle_cont.a:x} prot=0x{oracle_cont.b:x} "
            f"published={oracle_cont.c} region={signed64(oracle_cont.d)} "
            f"guard_reason={reason_text}"
        )
    if oracle_stack:
        print(
            "oracle_stack="
            f"rsp=0x{oracle_stack.a:x} w0=0x{oracle_stack.b:x} "
            f"w1=0x{oracle_stack.c:x} w2=0x{oracle_stack.d:x}"
        )

    print("\nDiagnosis")
    print("---------")
    if guarded_status == -94 or (test_after and signed64(test_after.a) == -94):
        reason = oracle_meta.a if oracle_meta else 0
        reason_text = BOUNDARY_GUARD_REASONS.get(reason, str(reason))
        print("Return-continuation guard stopped the run before a guest crash.")
        print(f"Next fix: explain why the first HLE continuation is {reason_text}.")
        if oracle_cont:
            if oracle_cont.b == 0xFFFFFFFF:
                print("The continuation is not in native executable mappings; compare callsite/PLT/GOT against shadPS4.")
            elif oracle_cont.c == 0:
                print("The continuation is mapped but not published; fix native executable publication coverage.")
            else:
                print("The continuation is mapped/published; inspect the guard reason and saved frame words.")
        else:
            print("No oracle boundary event was found; rerun with ELISA_GUEST_EXEC_DEBUG_TRACE=1.")
    elif guarded_status == -3 and first_boundary == 0:
        print("Timeout before the first HLE boundary.")
        pc_valid = fact_int(facts, "guest_exec_signal_pc_valid")
        pc = fact_int(facts, "guest_exec_signal_pc")
        if pc_valid and pc:
            print(f"Timeout snapshot captured child PC 0x{pc:x}; disassemble that guest window next.")
        else:
            print("No child PC captured; check the timeout SIGTRAP snapshot path before guessing at guest code.")
    elif last_signal:
        pc = fact_int(facts, "guest_exec_signal_pc")
        print(f"Guest/host signal captured: signal={last_signal} pc=0x{pc:x}.")
        print("Next fix: disassemble the captured PC and compare native backing bytes with the image.")
    elif test_after and signed64(test_after.a) == 0:
        print("Guard returned success and the crash happened in the host test tail.")
        print("Next fix: move artifact/tape emission behind a host-only cleanup path and inspect post-guard helpers.")
    else:
        print("No known terminal pattern matched; inspect the last GEWD events and artifact summary below.")

    if events:
        print(f"\nGEWD tail (events={len(events)})")
        print("-----------------------")
        for event in events[-args.tail :]:
            print_event(event)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
