#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LEDGER_PATH = ROOT / "parity_ledger.json"

ALLOWED_STATUSES = {
    "Implemented",
    "Native-Elisa",
    "Stub-Parity",
    "External-C-ABI",
    "Temporary-Cpp-Bridge",
    "AeroLibFallback",
    "Missing",
    "Not-Applicable-External",
}
ALLOWED_EXTERNAL_REASONS = {
    "external-library",
    "os-runtime-api",
    "temporary-project-bridge-approved",
}


def fail(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


def main() -> int:
    if not LEDGER_PATH.exists():
        return fail(f"missing ledger: {LEDGER_PATH}")

    raw = json.loads(LEDGER_PATH.read_text())
    active_targets = raw.get("active_targets", [])
    if not active_targets:
        return fail("parity ledger has no active_targets")

    summary_total = 0
    summary_missing = 0

    for target in active_targets:
        name = target.get("name", "<unnamed>")
        required = target.get("required_symbols", [])
        entries = target.get("entries", [])
        by_symbol: dict[str, dict] = {}

        for entry in entries:
            symbol = entry.get("symbol")
            if not symbol:
                return fail(f"{name}: entry without symbol")
            if symbol in by_symbol:
                return fail(f"{name}: duplicate symbol entry: {symbol}")
            by_symbol[symbol] = entry

            status = entry.get("status")
            if status not in ALLOWED_STATUSES:
                return fail(f"{name}:{symbol}: invalid status {status!r}")

            if status in {"Implemented", "Native-Elisa"}:
                tests = entry.get("tests", [])
                if not isinstance(tests, list) or len(tests) == 0:
                    return fail(
                        f"{name}:{symbol}: {status} entries must provide at least one coverage test tag"
                    )

            if status == "External-C-ABI":
                reason = entry.get("boundary_reason")
                if reason not in ALLOWED_EXTERNAL_REASONS:
                    return fail(
                        f"{name}:{symbol}: External-C-ABI must use boundary_reason "
                        f"{sorted(ALLOWED_EXTERNAL_REASONS)}, got {reason!r}"
                    )
                shim_owner = entry.get("shim_owner", "")
                if not shim_owner:
                    return fail(f"{name}:{symbol}: External-C-ABI entry requires shim_owner")

            if status == "Temporary-Cpp-Bridge":
                blocker = entry.get("bridge_blocker", "")
                plan = entry.get("retirement_plan", "")
                notes = str(entry.get("notes", "")).strip()
                if not blocker:
                    return fail(f"{name}:{symbol}: Temporary-Cpp-Bridge entry requires bridge_blocker")
                if not plan:
                    return fail(f"{name}:{symbol}: Temporary-Cpp-Bridge entry requires retirement_plan")
                if not notes:
                    return fail(f"{name}:{symbol}: Temporary-Cpp-Bridge entry requires retirement notes in notes")

            if status == "Missing":
                milestone = entry.get("milestone", "")
                if not milestone:
                    return fail(f"{name}:{symbol}: Missing entry requires milestone")

        for symbol in required:
            if symbol not in by_symbol:
                return fail(f"{name}: required symbol is unclassified: {symbol}")

        summary_total += len(required)
        summary_missing += sum(1 for s in required if by_symbol[s].get("status") == "Missing")

    if "--summary" in sys.argv:
        print(f"targets: {len(active_targets)}")
        print(f"required symbols: {summary_total}")
        print(f"missing symbols: {summary_missing}")
        pct = 100.0 if summary_total == 0 else ((summary_total - summary_missing) * 100.0 / summary_total)
        print(f"ledger completeness: {pct:.1f}%")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
