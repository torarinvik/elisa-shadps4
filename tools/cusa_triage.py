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
    23: "PC_SAMPLE",
    24: "GUARD_LONGJMP",
    124: "WATCH_HIT",
    142: "WATCH_ARM",
    110: "SSE4A_SIGILL_ENTER",
    111: "SSE4A_EXTRQ_HANDLED",
    112: "SSE4A_INSERTQ_HANDLED",
    113: "SSE4A_UNHANDLED",
    120: "ORACLE_BOUNDARY_CONT",
    121: "ORACLE_BOUNDARY_STACK",
    122: "ORACLE_BOUNDARY_META",
    138: "NATIVE_REGION_META",
    139: "NATIVE_REGION_ENTRY",
    140: "NATIVE_REGION_TRUNC",
    141: "LIBC_MALLOC_NULL",
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

LONGJMP_SOURCES = {
    1: "debug-trap-abort",
    2: "boundary-guard",
    3: "alarm",
    4: "pc-sampler",
    5: "signal",
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


# ---------------------------------------------------------------------------
# Symbolization: classify a guest/host address against the region table and the
# HLE thunk targets recorded on the tape, so raw hex becomes "region[k]+off (RW)"
# or "HLE thunk <nid> (<name>)" without leaving the triage output.
# ---------------------------------------------------------------------------

# Cache for best-effort NID -> human name resolution (greps the in-repo NID DBs).
_NID_NAME_CACHE: dict[str, str] = {}


def _nid_db_paths() -> list[Path]:
    here = Path(__file__).resolve().parent.parent  # elisa-shad-ps4-from-scratch/
    return [
        here / "core" / "aerolib" / "aerolib_nids.elisa",
        here / "core" / "libraries" / "kernel" / "kernel_nids.elisa",
    ]


def resolve_nid_name(nid: str) -> str:
    if not nid or nid in _NID_NAME_CACHE:
        return _NID_NAME_CACHE.get(nid, "")
    name = ""
    needle = '"' + nid + '"'
    for path in _nid_db_paths():
        if not path.exists():
            continue
        try:
            for line in path.read_text(errors="replace").splitlines():
                if needle in line:
                    # NidEntry("<nid>", "<name>") / KernelNidEntry("<nid>", "<lib>", "<lib>", "<name>", ...)
                    parts = re.findall(r'"([^"]*)"', line)
                    if parts and parts[0] == nid and len(parts) >= 2:
                        name = parts[-1] if "Kernel" in line else parts[1]
                        # KernelNidEntry: name is the 4th string; NidEntry: 2nd.
                        if "KernelNidEntry" in line and len(parts) >= 4:
                            name = parts[3]
                        break
            if name:
                break
        except OSError:
            continue
    _NID_NAME_CACHE[nid] = name
    return name


def _decode_nid_words(b: int, c: int) -> str:
    raw = struct.pack("<Q", b & 0xFFFFFFFFFFFFFFFF) + struct.pack("<Q", c & 0xFFFFFFFFFFFFFFFF)
    return raw.split(b"\x00")[0].decode("ascii", "replace")


def build_hle_targets(events: list[Event]) -> dict[int, str]:
    """Map HLE thunk target address -> NID string from RING_SYMBOL/RING_TARGET pairs."""
    nid_by_id: dict[int, str] = {}
    targets: dict[int, str] = {}
    for e in events:
        if e.kind == 129:  # HLE_RING_SYMBOL: a=id, b/c=nid bytes
            nid_by_id[e.a] = _decode_nid_words(e.b, e.c)
        elif e.kind == 130:  # HLE_RING_TARGET: a=id, b=target
            nid = nid_by_id.get(e.a, "")
            if e.b:
                targets[e.b] = nid
    return targets


def build_regions(events: list[Event]) -> list[tuple[int, int, int, int]]:
    """Return [(base, size, prot, table_index)] from the most recent region dump."""
    meta = latest(events, 138)
    if not meta:
        return []
    window = 40
    out = []
    for e in events:
        if e.kind == 139 and meta.seq - window <= e.seq < meta.seq:
            out.append((e.a, e.b, e.c, e.d & 0xFFFFFFFF))
    out.sort(key=lambda r: r[0])
    return out


_PROT_BITS = {0: "---", 1: "r--", 3: "rw-", 5: "r-x", 7: "rwx"}


def is_poison_addr(value: int) -> bool:
    """Recognize the ELISA_POISON_UNINIT fill (0xBE bytes). A pointer read from poisoned
    malloc memory is 0xBEBEBEBEBEBEBEBE; dereferencing it faults at 0xBEBE..+offset, whose
    top 5 bytes stay 0xBE for any offset < 0x1000000."""
    return (value >> 24) == 0xBEBEBEBEBE


def symbolize(addr: int, regions, hle_targets: dict[int, str]) -> str:
    if addr == 0:
        return "NULL"
    if is_poison_addr(addr):
        return "POISONED uninitialized memory (0xBE fill — read before init)"
    if addr in hle_targets:
        nid = hle_targets[addr]
        name = resolve_nid_name(nid)
        return f"HLE thunk {nid}" + (f" ({name})" if name else "")
    # Regions can overlap (e.g. a PROT_NONE reservation under a committed RW range);
    # pick the smallest containing region as the most specific backing.
    containing = [r for r in regions if r[0] <= addr < r[0] + r[1]]
    if containing:
        base, size, prot, idx = min(containing, key=lambda r: r[1])
        protname = _PROT_BITS.get(prot, f"0x{prot:x}")
        extra = ""
        if len(containing) > 1:
            extra = f" (+{len(containing) - 1} overlapping)"
        return f"region[{idx}]+0x{addr - base:x} ({protname}){extra}"
    # Coarse address-space classification when no region matches.
    if addr < 0x10000:
        return "near-null/unmapped"
    if 0x700000000000 <= addr < 0x800000000000:
        return "host/thunk space (unmapped in table)"
    return "unmapped"


# ---------------------------------------------------------------------------
# Fault explainer: reconstruct the bytes around the fault PC from the FAULT_INSN
# events, disassemble them, annotate the faulting instruction with live register
# values, and do one level of backward dataflow to find where the bad pointer
# was loaded from. This is the manual GPRS+capstone+dataflow trail, automated.
# ---------------------------------------------------------------------------

_REG64 = {
    "rax": "rax", "eax": "rax", "ax": "rax", "al": "rax",
    "rbx": "rbx", "ebx": "rbx", "bx": "rbx", "bl": "rbx",
    "rcx": "rcx", "ecx": "rcx", "cx": "rcx", "cl": "rcx",
    "rdx": "rdx", "edx": "rdx", "dx": "rdx", "dl": "rdx",
    "rdi": "rdi", "edi": "rdi",
    "rsi": "rsi", "esi": "rsi",
}


def _norm_reg(name: str) -> str:
    return _REG64.get(name.strip(), name.strip())


def _mem_operand(op_str: str):
    """Parse a `[base + disp]` / `[rip + disp]` memory operand. Returns (base, disp) or None."""
    m = re.search(r"\[([^\]]+)\]", op_str)
    if not m:
        return None
    inner = m.group(1).replace(" ", "")
    mm = re.match(r"^([a-z0-9]+)(?:([+\-])(0x[0-9a-f]+|\d+))?$", inner)
    if not mm:
        return None
    base = mm.group(1)
    disp = 0
    if mm.group(3):
        disp = int(mm.group(3), 0)
        if mm.group(2) == "-":
            disp = -disp
    return base, disp


def watch_section(events: list[Event], regions, hle_targets) -> list[str]:
    """Decode memory-watchpoint hits (GE_EV_WATCH_HIT, kind 124).

    Three recorded shapes:
      * page-watch write fault: a=watched_addr, b=writer_pc, c=hit#, d=sp
      * page-watch stored value (post single-step): a=addr, b=value, c=hit#, d=0xA10E
      * polling value-watch:     a=addr, b=value, c=poll_site, d=hit_seq
    """
    hits = [e for e in events if e.kind == 124]
    arms = [e for e in events if e.kind == 142]
    if not hits and not arms:
        return []
    out = ["", "Memory watchpoint", "-----------------"]
    # The fault-time re-emit carries d = total writes observed (survives ring wrap); the
    # setup-time arm has d=0. Prefer the max d across arm events as the write count.
    arm_writes = max((e.d for e in arms), default=0) if arms else 0
    for e in arms[:1] + (arms[-1:] if len(arms) > 1 else []):
        mode = "PROT_NONE (reads+writes)" if e.c == 0 else "PROT_READ (writes)"
        out.append(
            f"  armed at 0x{e.a:x} [{symbolize(e.a, regions, hle_targets)}] page=0x{e.b:x} "
            f"trap={mode} writes_observed={e.d}"
        )
    saw_write = arm_writes > 0
    for e in hits:
        if e.d == 0xA10E:
            out.append(
                f"  store: [0x{e.a:x} {symbolize(e.a, regions, hle_targets)}] <- 0x{e.b:x} (write #{e.c})"
            )
            saw_write = True
        elif e.c != 0 and 0x10000 <= e.b:  # writer PC present
            out.append(
                f"  write #{e.c} to 0x{e.a:x} by PC 0x{e.b:x} [{symbolize(e.b, regions, hle_targets)}] sp=0x{e.d:x}"
            )
            saw_write = True
        else:
            out.append(
                f"  value-watch: [0x{e.a:x} {symbolize(e.a, regions, hle_targets)}] == 0x{e.b:x} "
                f"at poll_site={e.c} seq={e.d}"
            )
    if arms and not saw_write:
        out.append(
            "  No WRITE to the watched slot was recorded before the run ended → the slot is "
            "likely never initialized (missing relocation / init_array / HLE write)."
        )
    return out


def explain_fault(events: list[Event], regions, hle_targets) -> list[str]:
    sig = latest(events, 11)
    if not sig:
        return []
    fault_pc, fault_addr, sp = sig.b, sig.d, sig.c
    out = [
        "",
        "Fault explainer",
        "---------------",
        f"signal={sig.a} pc=0x{fault_pc:x} [{symbolize(fault_pc, regions, hle_targets)}] "
        f"fault_addr=0x{fault_addr:x} [{symbolize(fault_addr, regions, hle_targets)}] sp=0x{sp:x}",
    ]
    regs: dict[str, int] = {}
    gprs = latest(events, 134)
    if gprs:
        regs.update({"rax": gprs.a, "rbx": gprs.b, "rcx": gprs.c, "rdx": gprs.d})
    # rdi is carried in the pc-16 FAULT_INSN event's `d` slot.
    for e in events:
        if e.kind == 135 and e.a == (fault_pc - 16) & 0xFFFFFFFFFFFFFFFF:
            regs["rdi"] = e.d
    if regs:
        out.append("  regs: " + " ".join(f"{k}=0x{v:x}" for k, v in sorted(regs.items())))

    # Reconstruct a byte window from the FAULT_INSN events (kind 135).
    word_at: dict[int, int] = {}
    for e in events:
        if e.kind != 135:
            continue
        word_at[e.a] = e.b
        word_at[e.a + 8] = e.c
        if e.a != (fault_pc - 16) & 0xFFFFFFFFFFFFFFFF:  # that slot holds rdi, not a code word
            word_at[e.a + 16] = e.d
    if not word_at:
        return out

    try:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_64
    except Exception:
        out.append("  (install `capstone` for disassembly + dataflow: pip install capstone)")
        return out

    # The recorded word addresses are at fault_pc-48/-24/-16/pc (not 8-aligned to each
    # other in general), so build contiguous runs from the actual keys and pick the run
    # that brackets fault_pc.
    addrs = sorted(word_at)
    runs: list[list[int]] = []
    for ad in addrs:
        if runs and ad == runs[-1][-1] + 8:
            runs[-1].append(ad)
        else:
            runs.append([ad])
    run = next((r for r in runs if r[0] <= fault_pc < r[-1] + 8), None)
    if not run:
        out.append("  (fault PC not covered by recorded instruction words)")
        return out
    base = run[0]
    buf = bytearray()
    for ad in run:
        buf += struct.pack("<Q", word_at[ad] & 0xFFFFFFFFFFFFFFFF)

    md = Cs(CS_ARCH_X86, CS_MODE_64)
    # x86 is not self-synchronizing; try start offsets until an instruction lands on fault_pc.
    chosen = None
    for start_off in range(0, min(len(buf), 48)):
        insns = list(md.disasm(bytes(buf[start_off:]), base + start_off))
        if any(i.address == fault_pc for i in insns):
            chosen = insns
            break
    if not chosen:
        out.append("  (could not align disassembly to fault PC)")
        return out

    fault_idx = next(i for i, ins in enumerate(chosen) if ins.address == fault_pc)
    for ins in chosen[max(0, fault_idx - 4): fault_idx + 1]:
        mark = "  >>" if ins.address == fault_pc else "    "
        out.append(f"{mark} 0x{ins.address:x}: {ins.mnemonic} {ins.op_str}")

    fault_ins = chosen[fault_idx]
    mem = _mem_operand(fault_ins.op_str)
    if not mem:
        return out
    base_reg, disp = mem
    base_reg = _norm_reg(base_reg)
    out.append(
        f"  faulting access: [{base_reg}{'+' if disp >= 0 else '-'}0x{abs(disp):x}] "
        f"-> 0x{fault_addr:x}"
    )
    if base_reg in regs and regs[base_reg] < 0x10000:
        out.append(f"  ROOT: base register {base_reg}=0x{regs[base_reg]:x} is null/near-null.")
        # One level of backward dataflow: find the last def of base_reg before the fault.
        for ins in reversed(chosen[:fault_idx]):
            dst = ins.op_str.split(",")[0].strip()
            if _norm_reg(dst) != base_reg:
                continue
            out.append(f"  defined by: 0x{ins.address:x}: {ins.mnemonic} {ins.op_str}")
            src = _mem_operand(ins.op_str)
            if ins.mnemonic.startswith("mov") and src:
                sbase, sdisp = src
                if sbase == "rip":
                    eff = ins.address + ins.size + sdisp
                    out.append(
                        f"  => {base_reg} loaded from global 0x{eff:x} "
                        f"[{symbolize(eff, regions, hle_targets)}] which held NULL."
                    )
                elif _norm_reg(sbase) in regs:
                    eff = (regs[_norm_reg(sbase)] + sdisp) & 0xFFFFFFFFFFFFFFFF
                    out.append(
                        f"  => {base_reg} loaded from [{_norm_reg(sbase)}+0x{sdisp:x}]=0x{eff:x} "
                        f"[{symbolize(eff, regions, hle_targets)}] which held NULL."
                    )
                out.append(
                    "  Next: find who initializes that slot (relocation / init_array / HLE). "
                    "If it is never written, that is the bug."
                )
            break
    return out


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
    sample_count = fact_int(facts, "guest_exec_pc_sample_count")
    sample_last_pc = fact_int(facts, "guest_exec_pc_sample_last_pc")
    sse4a_extrq = fact_int(facts, "guest_exec_sse4a_extrq_handled_count")
    sse4a_insertq = fact_int(facts, "guest_exec_sse4a_insertq_handled_count")
    sse4a_last_pc = fact_int(facts, "guest_exec_sse4a_last_pc")
    sse4a_last_word = fact_int(facts, "guest_exec_sse4a_last_word")

    test_after = latest(events, 22)
    oracle_cont = latest(events, 120)
    oracle_stack = latest(events, 121)
    oracle_meta = latest(events, 122)
    signal_event = latest(events, 11) or latest(events, 13)
    sse4a_unhandled = latest(events, 113)
    longjmp_event = latest(events, 24)
    sample_events = [event for event in events if event.kind == 23]
    hle_return = latest(events, 8) or latest(events, 9)
    if sample_events and sample_count == 0:
        sample_count = len(sample_events)
        sample_last_pc = sample_events[-1].b

    print("CUSA triage")
    print("===========")
    print(f"entry=0x{entry:x} entry_native_prot=0x{native_prot:x}")
    print(
        f"guarded_status={guarded_status} last_signal={last_signal} "
        f"first_boundary={first_boundary} reason={boundary_reason_name or '?'}"
    )
    if runtime_symbol:
        print(f"first_hle={runtime_module}:{runtime_symbol} return=0x{runtime_ret:x}")
    tls_base = fact_int(facts, "tls_primary_base")
    tls_size = fact_int(facts, "tls_primary_size")
    if tls_base and tls_size:
        print(
            f"tls_primary=0x{tls_base:x}..0x{tls_base + tls_size:x} "
            f"arg0_in_tls={fact_int(facts, 'runtime_hle_arg0_in_tls_primary')} "
            f"trap_in_tls={fact_int(facts, 'debug_trap_value_in_tls_primary')} "
            f"boundary_in_tls={fact_int(facts, 'boundary_value_in_tls_primary')}"
        )
    if sample_count:
        print(f"pc_samples={sample_count} last_pc=0x{sample_last_pc:x}")
    if sse4a_extrq or sse4a_insertq or sse4a_last_pc or sse4a_unhandled:
        print(
            "sse4a="
            f"extrq={sse4a_extrq} insertq={sse4a_insertq} "
            f"last_pc=0x{sse4a_last_pc:x} last_word=0x{sse4a_last_word:x}"
        )
        if sse4a_unhandled:
            print(
                "sse4a_unhandled="
                f"pc=0x{sse4a_unhandled.a:x} word=0x{sse4a_unhandled.b:x} reason={sse4a_unhandled.c}"
            )
    if test_after:
        print(
            "test_after_guard="
            f"status={signed64(test_after.a)} signal={test_after.b} fs_owner={test_after.c} last_status={signed64(test_after.d)}"
        )
    if longjmp_event:
        source = LONGJMP_SOURCES.get(longjmp_event.b, str(longjmp_event.b))
        print(
            "guard_longjmp="
            f"status={signed64(longjmp_event.a)} source={source} "
            f"value=0x{longjmp_event.c:x} previous_last_status={signed64(longjmp_event.d)}"
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
    elif hle_return:
        print(f"hle_return_event=continuation=0x{hle_return.c:x} rsp=0x{hle_return.d:x}")
    if oracle_stack:
        print(
            "oracle_stack="
            f"rsp=0x{oracle_stack.a:x} w0=0x{oracle_stack.b:x} "
            f"w1=0x{oracle_stack.c:x} w2=0x{oracle_stack.d:x}"
        )

    region_meta = latest(events, 138)
    if region_meta:
        found = region_meta.d
        found_text = "none" if found == 0xFFFFFFFF else str(found)
        print(
            "native_regions="
            f"active={region_meta.a} lookup=0x{region_meta.b:x} "
            f"lookup_size={region_meta.c} found_index={found_text}"
        )
        # The META is re-emitted last (newest) so it survives a ring wrap; the dump's
        # ENTRY events therefore sit just below it in seq. Gather entries in a small
        # window ending at the META (the per-dump entry cap is 24, plus a few extras).
        window = 40
        region_entries = [
            e
            for e in events
            if e.kind == 139 and region_meta.seq - window <= e.seq < region_meta.seq
        ]
        region_entries.sort(key=lambda e: e.a)
        for e in region_entries:
            table_index = e.d & 0xFFFFFFFF
            active = (e.d >> 32) & 0xFFFFFFFF
            end = e.a + e.b
            contains = "  <-- contains lookup" if e.a <= region_meta.b < end else ""
            print(
                f"  region[{table_index}] 0x{e.a:x}..0x{end:x} "
                f"size=0x{e.b:x} prot=0x{e.c:x} active={active}{contains}"
            )
        region_trunc = latest(events, 140)
        if region_trunc and region_meta.seq - window <= region_trunc.seq < region_meta.seq:
            print(
                f"  (region dump truncated: emitted {region_trunc.a} of {region_trunc.b} active)"
            )

    regions = build_regions(events)
    hle_targets = build_hle_targets(events)
    for line in explain_fault(events, regions, hle_targets):
        print(line)
    for line in watch_section(events, regions, hle_targets):
        print(line)

    print("\nDiagnosis")
    print("---------")
    if guarded_status == -94 or (test_after and signed64(test_after.a) == -94):
        reason_text = BOUNDARY_GUARD_REASONS.get(oracle_meta.a, str(oracle_meta.a)) if oracle_meta else "unknown"
        print("Return-continuation guard stopped the run before a guest crash.")
        print(f"Next fix: explain why the first HLE continuation is {reason_text}.")
        if fact_int(facts, "runtime_hle_arg0_in_tls_primary") or fact_int(facts, "first_bad_arg0_in_tls_primary") or fact_int(facts, "debug_trap_value_in_tls_primary"):
            print("TLS provenance lit up: an HLE/debug pointer falls inside the host-allocated primary TLS block.")
            print("Next fix: allocate/register primary TLS through guest-exec memory, matching shadPS4.")
        if oracle_cont:
            if oracle_cont.b == 0xFFFFFFFF:
                print("The continuation is not in native executable mappings; compare callsite/PLT/GOT against shadPS4.")
            elif oracle_cont.c == 0:
                print("The continuation is mapped but not published; fix native executable publication coverage.")
            else:
                print("The continuation is mapped/published; inspect the guard reason and saved frame words.")
        elif hle_return:
            print(f"Fallback from GEWD: first HLE continuation was 0x{hle_return.c:x}.")
            print("Next fix: inspect the callsite_probe/address_probe for that continuation and compare against shadPS4.")
        else:
            print("No oracle boundary event was found; rerun with ELISA_GUEST_EXEC_DEBUG_TRACE=1.")
    elif guarded_status == -3 and first_boundary == 0:
        print("Timeout before the first HLE boundary.")
        pc_valid = fact_int(facts, "guest_exec_signal_pc_valid")
        pc = fact_int(facts, "guest_exec_signal_pc")
        if sample_count and sample_last_pc:
            print(f"PC sampler captured {sample_count} sample(s); last PC is 0x{sample_last_pc:x}.")
            print("Next fix: disassemble the sampled loop and compare its inputs with shadPS4.")
        elif pc_valid and pc:
            print(f"Timeout snapshot captured child PC 0x{pc:x}; disassemble that guest window next.")
        else:
            print("No child PC captured; check the timeout SIGTRAP snapshot path before guessing at guest code.")
    elif last_signal == 4 and sse4a_unhandled:
        print("Rosetta raised SIGILL on an SSE4A-like instruction the fallback did not handle.")
        print("Next fix: decode the word above and add the missing SSE4A form to the signal-time fallback or prepatcher.")
    elif last_signal:
        pc = fact_int(facts, "guest_exec_signal_pc")
        print(f"Guest/host signal captured: signal={last_signal} pc=0x{pc:x}.")
        print("Next fix: disassemble the captured PC and compare native backing bytes with the image.")
    elif sse4a_extrq or sse4a_insertq:
        print("SSE4A fallback handled at least one Rosetta SIGILL and execution continued past it.")
        print("Next fix: inspect the next terminal condition; SSE4A was not the final blocker for this run.")
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
