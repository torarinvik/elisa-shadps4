#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_PATH = ROOT / "imgui_big_picture_parity_matrix.json"
EXPECTED = {"MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"}


def fail(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


def main() -> int:
    if not JSON_PATH.exists():
        return fail(f"missing matrix: {JSON_PATH}")

    data = json.loads(JSON_PATH.read_text())
    counts = data.get("counts", {})
    entries = data.get("entries", [])

    for key in EXPECTED:
        if key not in counts:
            return fail(f"missing count bucket: {key}")

    unresolved = [e for e in entries if e.get("status") != "MATCHED"]

    if "--summary" in sys.argv:
        total = sum(int(v) for v in counts.values())
        print(f"total symbols: {total}")
        for key in ["MATCHED", "MISMATCH_BEHAVIOR", "MISSING_IN_ELISA", "UNTESTED_BEHAVIOR"]:
            print(f"{key}: {counts.get(key, 0)}")

    if unresolved:
        print("unresolved parity entries:", file=sys.stderr)
        for e in unresolved[:200]:
            print(f"- {e.get('symbol')}: {e.get('status')}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
