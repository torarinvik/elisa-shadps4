#!/usr/bin/env python3
"""Port review: for each `@port`-anchored Elisa function, show the oracle C++ source
next to the Elisa source, so faithfulness can be judged (and divergences fixed).

The provenance checker (port_provenance.py) guarantees the C++ a function is anchored
to hasn't *changed* since stamping; this tool surfaces the two bodies so a human/LLM
can verify they actually do the *same thing* (and re-stamp once reviewed).

    python3 tools/port_review.py core/libraries/kernel/memory.elisa
    python3 tools/port_review.py core/libraries/kernel/memory.elisa::sceKernelMapDirectMemory
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from port_provenance import ANNOT_RE, ORACLE_SRC, extract_symbol_source  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


def elisa_body(lines: list[str], def_idx: int) -> str:
    """Capture the Elisa `def` block starting at def_idx (its signature through the
    last line more-indented than the def)."""
    indent = len(lines[def_idx]) - len(lines[def_idx].lstrip())
    out = [lines[def_idx]]
    for j in range(def_idx + 1, len(lines)):
        ln = lines[j]
        if ln.strip() == "":
            out.append(ln)
            continue
        if (len(ln) - len(ln.lstrip())) <= indent:
            break
        out.append(ln)
    while out and out[-1].strip() == "":
        out.pop()
    return "\n".join(out)


def review(elisa_path: Path, only_symbol: str | None) -> int:
    lines = elisa_path.read_text().split("\n")
    rp = elisa_path.relative_to(ROOT).as_posix()
    shown = 0
    for i, line in enumerate(lines):
        m = ANNOT_RE.match(line)
        if not m:
            continue
        rel, sym = m["path"], m["symbol"]
        if only_symbol and sym != only_symbol:
            continue
        # the def is the next non-decorator, non-comment line
        d = i + 1
        while d < len(lines) and (lines[d].lstrip().startswith("@") or lines[d].strip() == ""):
            d += 1
        if d >= len(lines):
            continue
        cpp_text = (ORACLE_SRC / rel).read_text(errors="replace") if (ORACLE_SRC / rel).is_file() else ""
        cpp_src, status = extract_symbol_source(cpp_text, sym) if cpp_text else (None, "missing-file")
        bar = "=" * 92
        print(f"\n{bar}\n{rp}:{i+1}  <-  {rel}::{sym}\n{bar}")
        print(f"--- shadPS4 C++ ({rel}) [{status}] ---")
        print(cpp_src if cpp_src else f"(could not extract: {status})")
        print(f"\n--- Elisa ({rp}) ---")
        print(elisa_body(lines, d))
        shown += 1
    if shown == 0:
        print(f"no @port anchors found in {rp}"
              + (f" for symbol {only_symbol}" if only_symbol else ""), file=sys.stderr)
        return 1
    print(f"\n[{shown} anchored function(s) shown]")
    return 0


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    arg = sys.argv[1]
    if "::" in arg:
        path_str, sym = arg.split("::", 1)
    else:
        path_str, sym = arg, None
    p = (ROOT / path_str) if not Path(path_str).is_absolute() else Path(path_str)
    if not p.is_file():
        print(f"not found: {p}", file=sys.stderr)
        return 2
    return review(p, sym)


if __name__ == "__main__":
    raise SystemExit(main())
