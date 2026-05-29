#!/usr/bin/env python3
"""Port-provenance checker: verify each ported Elisa function still matches the
shadPS4 C++ it was ported from.

The shadPS4 repo is the single source of truth. A ported Elisa function is anchored
to its C++ origin with a one-line annotation directly above it:

    # @port core/libraries/kernel/process.cpp::sceKernelGetCompiledSdkVersion sha256=<hash> oracle@<sha>
    def sceKernelGetCompiledSdkVersion(ver: void&?) -> int:

`@port <oracle-relpath>::<symbol>` names the C++ definition in the oracle
(`<relpath>` is relative to shadPS4/src). `sha256=` is a content hash of that C++
symbol's source at the moment the port was last verified; `oracle@<sha>` records
the oracle commit then (informational).

This tool, in check mode (default), re-extracts each named C++ symbol from the
current oracle, hashes it, and compares:
  - OK       : hash matches  -> port is still in sync with the oracle
  - DRIFTED  : hash differs  -> the C++ changed since the port was verified; re-review
  - BROKEN   : file/symbol missing or ambiguous in the oracle -> fix the annotation

In --update mode it (re)stamps `sha256=`/`oracle@` for every annotation from the
current oracle, so adding a new anchor is: write the `# @port path::symbol` line,
run `--update`, commit. Exit code is nonzero if any DRIFTED/BROKEN in check mode.

Comments get verified once and then rot; this verifies continuously and gates CI.
"""
from __future__ import annotations

import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ORACLE_SRC = (ROOT.parent / "shadPS4" / "src").resolve()

ANNOT_RE = re.compile(
    r"^(?P<indent>\s*)#\s*@port\s+(?P<path>[^\s:]+)::(?P<symbol>[A-Za-z_][A-Za-z0-9_]*)"
    r"(?:\s+sha256=(?P<sha>[0-9a-f]+))?(?:\s+oracle@(?P<commit>[0-9a-f]+))?\s*$"
)


def oracle_head() -> str:
    try:
        return subprocess.run(
            ["git", "-C", str(ORACLE_SRC), "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, check=True,
        ).stdout.strip()
    except Exception:
        return "unknown"


def find_matching_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    n = len(text)
    while i < n:
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def extract_symbol_source(text: str, symbol: str) -> tuple[str | None, str]:
    """Return (source, status) for the single C++ definition of `symbol`.

    A definition is `<sig> symbol ( <args> ) <quals> { <body> }`. Declarations
    (ending in `;`) and call sites are ignored. status is "ok", "missing", or
    "ambiguous".
    """
    defs: list[str] = []
    for m in re.finditer(r"\b" + re.escape(symbol) + r"\s*\(", text):
        paren = text.index("(", m.start())
        close = find_matching_brace_paren(text, paren)
        if close < 0:
            continue
        j = close + 1
        # skip qualifiers / whitespace between ) and {  (const, noexcept, override...)
        while j < len(text) and text[j] not in "{;":
            j += 1
        if j >= len(text) or text[j] != "{":
            continue  # declaration or call, not a definition
        body_end = find_matching_brace(text, j)
        if body_end < 0:
            continue
        sig_start = text.rfind("\n", 0, m.start()) + 1
        defs.append(text[sig_start:body_end + 1])
    if not defs:
        return None, "missing"
    if len(defs) > 1:
        return None, "ambiguous"
    return defs[0], "ok"


def find_matching_brace_paren(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    n = len(text)
    while i < n:
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def normalize(src: str) -> str:
    # Tolerate only EOL/trailing-whitespace noise; real edits still change the hash.
    lines = [ln.rstrip() for ln in src.replace("\r\n", "\n").split("\n")]
    return "\n".join(lines).strip()


def hash_symbol(rel_path: str, symbol: str) -> tuple[str | None, str]:
    f = ORACLE_SRC / rel_path
    if not f.is_file():
        return None, "missing-file"
    src, status = extract_symbol_source(f.read_text(errors="replace"), symbol)
    if status != "ok":
        return None, status
    return hashlib.sha256(normalize(src).encode()).hexdigest()[:16], "ok"


def elisa_files() -> list[Path]:
    return [p for p in ROOT.rglob("*.elisa") if ".git" not in p.parts]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--update", action="store_true",
                    help="(re)stamp sha256/oracle@ for all @port annotations")
    ap.add_argument("--summary", action="store_true")
    args = ap.parse_args()

    if not ORACLE_SRC.exists():
        print(f"oracle not found: {ORACLE_SRC}", file=sys.stderr)
        return 2

    head = oracle_head()
    ok = drifted = broken = stamped = 0
    drift_msgs: list[str] = []

    for path in elisa_files():
        text = path.read_text()
        lines = text.split("\n")
        changed = False
        for i, line in enumerate(lines):
            m = ANNOT_RE.match(line)
            if not m:
                continue
            rel, sym, sha = m["path"], m["symbol"], m["sha"]
            cur, status = hash_symbol(rel, sym)
            rp = path.relative_to(ROOT).as_posix()
            if status != "ok":
                broken += 1
                drift_msgs.append(f"BROKEN  {rp}:{i+1}  {rel}::{sym} -> {status}")
                continue
            if args.update:
                lines[i] = f"{m['indent']}# @port {rel}::{sym} sha256={cur} oracle@{head}"
                if lines[i] != line:
                    changed = True
                    stamped += 1
            elif sha is None:
                broken += 1
                drift_msgs.append(f"BROKEN  {rp}:{i+1}  {rel}::{sym} -> unstamped (run --update)")
            elif sha == cur:
                ok += 1
            else:
                drifted += 1
                drift_msgs.append(
                    f"DRIFTED {rp}:{i+1}  {rel}::{sym}  recorded={sha} current={cur}")
        if changed:
            path.write_text("\n".join(lines))

    if args.update:
        print(f"stamped {stamped} @port annotation(s) against oracle@{head}")
        return 0

    for msg in drift_msgs:
        print(msg, file=sys.stderr)
    if args.summary or drift_msgs:
        print(f"port-provenance: {ok} OK, {drifted} DRIFTED, {broken} BROKEN "
              f"(oracle@{head})")
    return 1 if (drifted or broken) else 0


if __name__ == "__main__":
    raise SystemExit(main())
