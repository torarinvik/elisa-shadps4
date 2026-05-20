#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_PATH = ROOT / "shader_recompiler_ir_core_parity_matrix.json"


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate shader recompiler IR core parity matrix")
    parser.add_argument("--summary", action="store_true", help="print summary counts")
    args = parser.parse_args()

    if not JSON_PATH.exists():
        print(f"missing matrix file: {JSON_PATH.relative_to(ROOT)}")
        return 1

    data = json.loads(JSON_PATH.read_text())
    counts = data.get("counts", {})
    missing = int(counts.get("MISSING_IN_ELISA", 0))

    if args.summary:
        total = sum(int(v) for v in counts.values())
        print(f"shader_recompiler/ir core parity: total={total} matched={counts.get('MATCHED', 0)} missing={missing}")

    if missing != 0:
        print("IR core parity matrix has missing Elisa symbols")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
