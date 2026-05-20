#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_PATH = ROOT / "shader_recompiler_backend_spirv_parity_matrix.json"


def main() -> int:
    if not JSON_PATH.exists():
        print(f"missing parity matrix: {JSON_PATH}", file=sys.stderr)
        return 1

    data = json.loads(JSON_PATH.read_text())
    unresolved = [entry for entry in data.get("entries", []) if entry.get("status") != "MATCHED"]
    if unresolved:
        print("unresolved parity entries:", file=sys.stderr)
        for entry in unresolved[:80]:
            print(
                f"- {entry.get('symbol')} @ {entry.get('source_file')} => {entry.get('status')}",
                file=sys.stderr,
            )
        if len(unresolved) > 80:
            print(f"... and {len(unresolved) - 80} more", file=sys.stderr)
        return 1

    if "--summary" in sys.argv:
        counts = data.get("counts", {})
        total = sum(int(v) for v in counts.values())
        print(f"total symbols: {total}")
        print(f"matched: {counts.get('MATCHED', 0)}")
        print(f"missing: {counts.get('MISSING_COVERAGE', 0)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
