#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


CODE_SUFFIXES = {".cpp", ".h", ".hpp", ".inl", ".S", ".s"}


def iter_code_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if ".git" in path.parts:
            continue
        if path.is_file() and path.suffix in CODE_SUFFIXES:
            files.append(path.relative_to(root))
    return sorted(files)


def iter_elisa_basenames(root: Path) -> set[Path]:
    names: set[Path] = set()
    for path in root.rglob("*.elisa"):
        if ".git" in path.parts:
            continue
        names.add(path.relative_to(root).with_suffix(""))
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description="Report C++/header files without same-basename Elisa ports.")
    parser.add_argument("--check", action="store_true", help="exit non-zero when any code file is uncovered")
    parser.add_argument("--list-missing", action="store_true", help="print every uncovered code file")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent
    code_files = iter_code_files(root)
    elisa_basenames = iter_elisa_basenames(root)
    missing = [path for path in code_files if path.with_suffix("") not in elisa_basenames]
    covered = len(code_files) - len(missing)
    percent = 100.0 if not code_files else covered * 100.0 / len(code_files)

    print(f"code files: {len(code_files)}")
    print(f"covered by same-basename .elisa: {covered}")
    print(f"missing same-basename .elisa: {len(missing)}")
    print(f"file coverage: {percent:.2f}%")

    if args.list_missing:
        for path in missing:
            print(path)

    return 1 if args.check and missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
