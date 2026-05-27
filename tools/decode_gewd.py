#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
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
    20: "MAIN_RUN_BEGIN",
    21: "MAIN_RUN_END",
    30: "MODULE_START_BEGIN",
    31: "MODULE_START_END",
    32: "MODULE_INIT_ENTRY_BEGIN",
    33: "MODULE_INIT_ENTRY_AFTER_INSTALL",
    34: "MODULE_INIT_ENTRY_AFTER_CALL",
    35: "MODULE_INIT_ENTRY_AFTER_RESTORE",
    36: "MODULE_INIT_ARRAY_BEGIN",
    37: "MODULE_INIT_ARRAY_AFTER_INSTALL",
    38: "MODULE_INIT_ARRAY_AFTER_CALL",
    39: "MODULE_INIT_ARRAY_AFTER_RESTORE",
    50: "MAIN_ENTRY_SYNC_BEGIN",
    51: "MAIN_ENTRY_SYNC_END",
    52: "MAIN_ENTRY_PREPARE_BEGIN",
    53: "MAIN_ENTRY_PREPARE_END",
    54: "MAIN_ENTRY_JUMP_BEGIN",
    55: "MAIN_ENTRY_JUMP_RETURNED",
    60: "DEP_CLOSURE_BEGIN",
    61: "DEP_CLOSURE_MODULE_BEGIN",
    62: "DEP_CLOSURE_DEP_BEGIN",
    63: "DEP_CLOSURE_DEP_END",
    64: "DEP_CLOSURE_MODULE_END",
    65: "DEP_CLOSURE_END",
    70: "EXEC_AFTER_DEP_CLOSURE",
    71: "EXEC_RELOC_ALL_BEGIN",
    72: "EXEC_RELOC_ALL_END",
    73: "EXEC_MEMORY_SETUP_BEGIN",
    74: "EXEC_MEMORY_SETUP_END",
    75: "EXEC_TLS_ALLOC_BEGIN",
    76: "EXEC_TLS_ALLOC_END",
    80: "RELOC_MODULE_BEGIN",
    81: "RELOC_MODULE_IMPORT_BEGIN",
    82: "RELOC_MODULE_IMPORT_END",
    83: "RELOC_MODULE_IMPORTS_DONE",
    84: "RELOC_TABLE_BEGIN",
    85: "RELOC_TABLE_ENTRY_BEGIN",
    86: "RELOC_TABLE_ENTRY_END",
    87: "RELOC_TABLE_END",
    88: "RELOC_MODULE_END",
    110: "SSE4A_SIGILL_ENTER",
    111: "SSE4A_EXTRQ_HANDLED",
    112: "SSE4A_INSERTQ_HANDLED",
    113: "SSE4A_UNHANDLED",
    1001: "MUTEX_INIT",
}


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode Elisa guest-exec emergency watchdog tape")
    parser.add_argument("path", nargs="?", default="/tmp/elisa_gewd.bin")
    parser.add_argument("--tail", type=int, default=80)
    args = parser.parse_args()

    data = Path(args.path).read_bytes()
    events = []
    for slot in range(min(256, len(data) // EVENT_SIZE)):
        seq, kind, phase, a, b, c, d = struct.unpack_from("<QI I QQQQ", data, slot * EVENT_SIZE)
        if seq:
            events.append((seq, slot, kind, phase, a, b, c, d))
    events.sort()

    print(f"events={len(events)} last_seq={events[-1][0] if events else 0}")
    for seq, slot, kind, phase, a, b, c, d in events[-args.tail:]:
        name = EVENT_NAMES.get(kind, str(kind))
        print(
            f"EV seq={seq:04d} slot={slot:03d} {name} "
            f"phase={phase} a=0x{a:x} b=0x{b:x} c=0x{c:x} d=0x{d:x}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
