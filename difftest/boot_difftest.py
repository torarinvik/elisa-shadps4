#!/usr/bin/env python3
"""Differential boot oracle: run the SAME PS4 game through shadPS4 (the reference) and the
from-scratch Elisa emulator, normalize each one's boot trace to a common event schema, and
report the FIRST divergence — which is, by construction, the place the Elisa emulator first
behaves differently from the proven reference.

This is the external-oracle complement to Elisa's in-language verification (contracts,
refinement types, @property): the native/FFI/guest-exec layer cannot be PROVEN, so we pin it
by differential equivalence against shadPS4. It is exactly the comparison that re-diagnosed
"CRASH #2" (entry value correct, segment load correct → divergence is post-load guest memory).

Usage:
    boot_difftest.py --shadps4 <bin> --game <eboot.bin> --elisa-runner <runner> [--elisa-test <name>]

Each backend is reduced to an ordered list of canonical events:
    ENTRY off=0x<entry - base>
    MODSTART module=<basename> phase=<begin|fallback|init-array|end>
so per-run addresses/bases (which legitimately differ) are abstracted away, and only the
*semantics* of the boot are compared.
"""
import argparse, re, subprocess, sys

EBOOT_BASE = 0x800000000

def run(cmd, timeout, env=None):
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
        return (p.stdout or "") + (p.stderr or "")
    except subprocess.TimeoutExpired as e:
        return (e.stdout or "") + (e.stderr or "") if isinstance(e.stdout, str) else ""

def canon_shadps4(log):
    ev = []
    for line in log.splitlines():
        m = re.search(r"program entry addr[ .]*:\s*0x([0-9a-fA-F]+)", line)
        if m:
            ev.append(("ENTRY", f"off=0x{int(m.group(1),16)-EBOOT_BASE:x}"))
        m = re.search(r"(\S+) using ELF entry fallback for module_start", line)
        if m:
            ev.append(("MODSTART", f"module={base(m.group(1))} phase=fallback"))
    return ev

def canon_elisa(log):
    ev = []
    for line in log.splitlines():
        m = re.search(r"\bentry=0x([0-9a-fA-F]+)\b.*\bbase=0x([0-9a-fA-F]+)", line)
        if m:
            ev.append(("ENTRY", f"off=0x{int(m.group(1),16)-int(m.group(2),16):x}"))
        m = re.search(r"modstart seq=\d+ phase=(\S+) module=(\S+)", line)
        if m:
            ph = {"begin":"begin","end":"end","elf-entry-fallback":"fallback"}.get(m.group(1), m.group(1))
            ev.append(("MODSTART", f"module={base(m.group(2))} phase={ph}"))
    return ev

def base(name):
    return re.sub(r"\.(prx|sprx|bin|elf)$", "", name.rsplit("/",1)[-1])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--shadps4", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--elisa-runner", required=True)
    ap.add_argument("--elisa-test", default="emulator_real_game_boot_smoke_guarded_game_entry_reports_host_status")
    ap.add_argument("--timeout", type=int, default=40)
    a = ap.parse_args()

    import os
    sp = canon_shadps4(run([a.shadps4, "-g", a.game], a.timeout, {**os.environ, "ELISA_HEADLESS":"1"}))
    el = canon_elisa(run([a.elisa_runner, a.elisa_test], a.timeout, {**os.environ, "ELISA_GUEST_EXEC_NO_FORK":"1"}))
    # The two backends emit events in different ORDER, so compare per CATEGORY, not positionally:
    #   - ENTRY: the main-module entry offset must agree exactly.
    #   - MODSTART(fallback): the ordered sequence of dependency modules that fall back to the ELF
    #     entry for module_start must agree (same libraries, same order).
    diverged = False
    se = [v for k, v in sp if k == "ENTRY"]
    ee = [v for k, v in el if k == "ENTRY"]
    print(f"ENTRY  shadPS4={se}  elisa={ee}")
    if not se or not ee or se[0] != ee[0]:
        print(f"  DIVERGENCE: main-module entry offset differs (shadPS4={se} elisa={ee})")
        diverged = True
    else:
        print(f"  OK entry {se[0]}")
    sf = [v for k, v in sp if k == "MODSTART" and "fallback" in v]
    ef = [v for k, v in el if k == "MODSTART" and "fallback" in v]
    print(f"MODSTART fallbacks  shadPS4={len(sf)}  elisa={len(ef)}")
    for i in range(max(len(sf), len(ef))):
        s = sf[i] if i < len(sf) else None
        e = ef[i] if i < len(ef) else None
        if s != e:
            print(f"  DIVERGENCE at modstart #{i}: shadPS4={s} elisa={e}")
            diverged = True
            break
        print(f"  [{i}] OK {s}")
    if diverged:
        print("RESULT: BOOT TRACES DIVERGE — investigate the Elisa emulator at the first differing event above.")
        sys.exit(1)
    print("RESULT: BOOT TRACES EQUIVALENT (entry offset + module-start fallback sequence).")
    sys.exit(0)

if __name__ == "__main__":
    main()
