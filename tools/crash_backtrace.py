#!/usr/bin/env python3
"""Symbolize the guest crash backtrace from the emergency tape.

The Elisa guest-exec signal handler writes a binary event tape to
/tmp/elisa_gewd.bin (256 fixed-size records). On a guest fault it records the
fault PC/address, the stack words around the faulting SP, and a 32-slot window
of the guest stack (GE_EV_STACK_DUMP). This script decodes those and flags every
stack word that lands inside a known guest module range, reconstructing the call
chain that led to the crash without touching live (post-fault, fragile) state.

Module ranges default to the CUSA07399 eboot; pass --module name=base:size to add
others (e.g. from the shadPS4 BOOT_ORACLE module lines or Elisa's artifacts).
"""
from __future__ import annotations

import argparse
import struct
import sys

REC_SIZE = 48  # seq:u64 kind:u32 phase:u32 a:u64 b:u64 c:u64 d:u64
KIND_SIGNAL_FIRED = 11
KIND_SIGNAL_STACK_WORDS = 17
KIND_STACK_DUMP = 123


def parse_tape(path: str) -> list[tuple]:
    data = open(path, "rb").read()
    out = []
    for i in range(len(data) // REC_SIZE):
        seq, kind, phase, a, b, c, d = struct.unpack_from("<QIIQQQQ", data, i * REC_SIZE)
        if seq > 0:
            out.append((seq, kind, phase, a, b, c, d))
    out.sort()
    return out


def parse_modules(specs: list[str]) -> list[tuple[str, int, int]]:
    mods = [("eboot.bin", 0x800000000, 0x2E04000)]
    for spec in specs or []:
        name, rng = spec.split("=", 1)
        base, size = rng.split(":", 1)
        mods.append((name, int(base, 0), int(size, 0)))
    return mods


def symbolize(value: int, mods: list[tuple[str, int, int]]) -> str | None:
    for name, base, size in mods:
        if base <= value < base + size:
            return f"{name}+{hex(value - base)}"
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--tape", default="/tmp/elisa_gewd.bin")
    ap.add_argument("--module", action="append", default=[],
                    help="name=base:size, e.g. libc=0x809380000:0xf8000")
    args = ap.parse_args()

    evs = parse_tape(args.tape)
    mods = parse_modules(args.module)

    for seq, kind, phase, a, b, c, d in evs:
        if kind == KIND_SIGNAL_FIRED:
            print(f"FAULT  sig={a} pc={hex(b)} sp={hex(c)} fault_addr={hex(d)}"
                  f"  pc->{symbolize(b, mods) or 'unmapped'}")
        elif kind == KIND_SIGNAL_STACK_WORDS:
            tc = {2: "ret-to-word_m1", 3: "call/jmp-to-word0", 4: "call/jmp-to-word1"}.get(d, str(d))
            print(f"STACK  word_m1={hex(a)} word0={hex(b)} word1={hex(c)} transfer_class={tc}")

    print("\nGUEST BACKTRACE (stack words landing in known modules):")
    found = False
    for seq, kind, phase, a, b, c, d in evs:
        if kind == KIND_STACK_DUMP:
            idx, addr, val = a, b, c
            sym = symbolize(val, mods)
            if sym:
                found = True
                print(f"  [sp+{idx*8:#04x}] {hex(addr)}: {hex(val)}  -> {sym}")
    if not found:
        print("  (no stack words resolved to known modules; pass --module for PRX ranges)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
