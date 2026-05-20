#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_COMPILER_DIR = ROOT.parent / "compiler"


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def run_step(name: str, cmd: list[str], cwd: Path, verbose: bool) -> bool:
    print(f"==> {name}")
    if verbose:
        print("+ " + " ".join(cmd))
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if proc.stdout:
        print(proc.stdout, end="" if proc.stdout.endswith("\n") else "\n")
    if proc.returncode == 0:
        print(f"OK {name}")
        return True

    print(f"FAIL {name}", file=sys.stderr)
    print("command: " + " ".join(cmd), file=sys.stderr)
    return False


def lowered_sources() -> list[Path]:
    sources: list[Path] = []
    for lowered in sorted((ROOT / "core" / "libraries").glob("**/*.lowered.elisa")):
        source = lowered.with_name(lowered.name.removesuffix(".lowered.elisa") + ".elisa")
        if source.exists():
            sources.append(source)
    return sources


def run_elisa_lowering(compiler_dir: Path, verbose: bool, check_drift: bool) -> bool:
    sources = lowered_sources()
    if not sources:
        print("No checked-in lowered Elisa files found.")
        return True

    ok = True
    for source in sources:
        lowered = source.with_name(source.name.removesuffix(".elisa") + ".lowered.elisa")
        before = lowered.read_bytes() if lowered.exists() else None
        cmd = ["go", "run", "./src", "-emit", "lowered", str(source)]
        step_ok = run_step(f"lower {rel(source)}", cmd, compiler_dir, verbose)
        if lowered.exists() and before is not None:
            after = lowered.read_bytes()
            if check_drift and after != before:
                print(f"lowered output drifted: {rel(lowered)}", file=sys.stderr)
                step_ok = False
            lowered.write_bytes(before)
        ok = step_ok and ok
    if ok:
        print(f"Elisa lowering check passed: {len(sources)} file(s).")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run the Elisa port validation suite: bridge syntax, ledgers, ABI guards, and lowering."
    )
    parser.add_argument(
        "--compiler-dir",
        default=os.environ.get("ELISA_COMPILER_DIR", str(DEFAULT_COMPILER_DIR)),
        help="Path to the Elisa compiler checkout.",
    )
    parser.add_argument("--skip-bridges", action="store_true", help="Skip C++ bridge syntax checks")
    parser.add_argument("--skip-ledger", action="store_true", help="Skip parity ledger checks")
    parser.add_argument("--skip-lowering", action="store_true", help="Skip Elisa lowering checks")
    parser.add_argument(
        "--check-lowered-drift",
        action="store_true",
        help="Fail if generated .lowered.elisa output differs from the checked-in file.",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Print commands before running")
    args = parser.parse_args()

    compiler_dir = Path(args.compiler_dir).resolve()
    ok = True

    if not args.skip_bridges:
        ok = run_step(
            "C++ bridge syntax",
            [sys.executable, "check_elisa_bridges.py"],
            ROOT,
            args.verbose,
        ) and ok

    if not args.skip_ledger:
        ok = run_step(
            "parity ledger",
            [sys.executable, "parity_ledger_check.py", "--summary"],
            ROOT,
            args.verbose,
        ) and ok
        ok = run_step(
            "parity ABI guard",
            [sys.executable, "parity_abi_check.py", "--summary"],
            ROOT,
            args.verbose,
        ) and ok
        ok = run_step(
            "system parity matrix generation",
            [sys.executable, "system_parity_matrix.py"],
            ROOT,
            args.verbose,
        ) and ok
        ok = run_step(
            "system parity matrix gate",
            [sys.executable, "system_parity_matrix_check.py", "--summary"],
            ROOT,
            args.verbose,
        ) and ok

    if not args.skip_lowering:
        if not (compiler_dir / "go.mod").exists():
            print(f"Missing Elisa compiler checkout: {compiler_dir}", file=sys.stderr)
            ok = False
        else:
            ok = run_elisa_lowering(compiler_dir, args.verbose, args.check_lowered_drift) and ok

    if ok:
        print("Elisa port validation passed.")
        return 0
    print("Elisa port validation failed.", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
