#!/usr/bin/env python3
from __future__ import annotations

import os
import platform
import json
import shutil
import subprocess
import sys
import argparse
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COMPILER_DIR = ROOT.parent / "compiler"


@dataclass(frozen=True)
class AbiCheck:
    name: str
    source: str
    extra_cppflags: tuple[str, ...] = ()
    optional: bool = False


def default_target_triple() -> str:
    machine = platform.machine().lower()
    if machine in {"amd64", "x86_64"}:
        arch = "x86_64"
    elif machine in {"arm64", "aarch64"}:
        arch = "aarch64"
    else:
        arch = machine or "unknown"
    if sys.platform == "darwin":
        return f"{arch}-apple-darwin"
    if sys.platform.startswith("linux"):
        return f"{arch}-unknown-linux-gnu"
    if sys.platform == "win32":
        return f"{arch}-pc-windows-msvc"
    return f"{arch}-unknown-unknown"


def ffmpeg_include_flags() -> tuple[str, ...]:
    candidates = [
        Path("/opt/homebrew/Cellar/ffmpeg/8.1.1/include"),
        Path("/opt/homebrew/include"),
        Path("/usr/local/include"),
        Path("/usr/include"),
    ]
    return tuple(f"-I{path}" for path in candidates if path.exists())


CHECKS = [
    AbiCheck("network sockets", "core/libraries/network/net_ffi.elisa"),
    AbiCheck("pad bridge DTOs", "core/libraries/pad/pad_ffi.elisa"),
    AbiCheck("voice bridge DTOs", "core/libraries/voice/voice_ffi.elisa"),
    AbiCheck("videodec bridge DTOs", "core/libraries/videodec/videodec_ffi.elisa"),
    AbiCheck("avplayer ffmpeg public prefixes", "core/libraries/avplayer/avplayer_ffmpeg_ffi.elisa", extra_cppflags=ffmpeg_include_flags(), optional=True),
]


def check_slug(check: AbiCheck) -> str:
    out = []
    for ch in check.name.lower():
        if ch.isalnum():
            out.append(ch)
        else:
            out.append("_")
    return "".join(out).strip("_")


def manifest_path(root: Path, check: AbiCheck, target_triple: str) -> Path:
    safe_target = target_triple.replace("-", "_").replace(".", "_")
    return root / f"{safe_target}__{check_slug(check)}.json"


def stable_manifest(manifest: dict[str, object]) -> str:
    return json.dumps(manifest, indent=2, sort_keys=True) + "\n"


def run_check(check: AbiCheck, target_triple: str) -> tuple[bool, str, dict[str, object] | None]:
    go = shutil.which("go")
    if go is None:
        return False, "go missing", None
    source = ROOT / check.source
    if not source.exists():
        return False, f"source missing: {check.source}", None
    env = os.environ.copy()
    # Keep the project include path relative to the compiler cwd so paths with
    # spaces do not get split by CPPFLAGS parsing.
    cppflags = ["-I../elisa-shad-ps4-from-scratch"]
    cppflags.extend(check.extra_cppflags)
    if env.get("CPPFLAGS"):
        cppflags.append(env["CPPFLAGS"])
    env["CPPFLAGS"] = " ".join(cppflags)
    cmd = [
        go,
        "run",
        "./src",
        "-emit",
        "c-bind-check-json",
        "-target-triple",
        target_triple,
        str(source),
    ]
    proc = subprocess.run(cmd, cwd=COMPILER_DIR, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    output = proc.stdout.strip()
    if proc.returncode != 0:
        return False, output, None
    json_start = output.find("{")
    json_end = output.rfind("}")
    json_payload = output[json_start:json_end + 1] if json_start >= 0 and json_end >= json_start else output
    try:
        manifest = json.loads(json_payload)
    except json.JSONDecodeError as exc:
        return False, f"invalid JSON manifest: {exc}\n{output}", None
    if manifest.get("target_triple") != target_triple:
        return False, f"target mismatch: manifest={manifest.get('target_triple')} expected={target_triple}", manifest
    structs = manifest.get("structs")
    if not isinstance(structs, list) or not structs:
        return False, "manifest contains no checked structs", manifest
    for struct in structs:
        if not isinstance(struct, dict):
            return False, "manifest contains non-object struct entry", manifest
        elisa_layout = struct.get("elisa")
        c_layout = struct.get("c")
        fields = struct.get("fields")
        if not isinstance(elisa_layout, dict) or not isinstance(c_layout, dict) or not isinstance(fields, list):
            return False, f"manifest has malformed layout for {struct.get('elisa_name')}", manifest
        if not struct.get("prefix"):
            if elisa_layout.get("size") != c_layout.get("size") or elisa_layout.get("align") != c_layout.get("align"):
                return False, f"layout mismatch for {struct.get('elisa_name')}", manifest
        for field in fields:
            if isinstance(field, dict) and field.get("elisa_offset") != field.get("c_offset"):
                return False, f"field offset mismatch for {struct.get('elisa_name')}.{field.get('name')}", manifest
    return True, output, manifest


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify emulator FFI DTO layouts against C headers for a target triple.")
    parser.add_argument("target_triple", nargs="?", default=default_target_triple())
    parser.add_argument("--write-manifests", type=Path, help="Write checked ABI manifests to this directory.")
    parser.add_argument("--golden-dir", type=Path, help="Compare checked ABI manifests with this directory.")
    args = parser.parse_args()
    target_triple = args.target_triple
    failures: list[str] = []
    for check in CHECKS:
        ok, output, manifest = run_check(check, target_triple)
        status = "PASS" if ok else ("SKIP" if check.optional else "FAIL")
        print(f"EMULATOR_ABI_SMOKE {status} name={check.name!r} target={target_triple}")
        if ok and manifest is not None:
            structs = manifest.get("structs", [])
            print(f"  manifest structs={len(structs)}")
            if isinstance(structs, list):
                for struct in structs[:6]:
                    if isinstance(struct, dict):
                        print(f"  c-bind-check-json: {struct.get('elisa_name')} matches {struct.get('c_name')}")
                if len(structs) > 6:
                    print(f"  ... {len(structs) - 6} more structs")
        elif output:
            for line in output.splitlines()[:12]:
                if "error:" in line or "mismatch" in line or "failed" in line:
                    print(f"  {line}")
        if not ok and not check.optional:
            failures.append(check.name)
        if ok and manifest is not None and args.write_manifests is not None:
            args.write_manifests.mkdir(parents=True, exist_ok=True)
            manifest_path(args.write_manifests, check, target_triple).write_text(stable_manifest(manifest))
        if ok and manifest is not None and args.golden_dir is not None:
            path = manifest_path(args.golden_dir, check, target_triple)
            if not path.exists():
                print(f"  missing golden manifest: {path}")
                if not check.optional:
                    failures.append(check.name + ":missing-golden")
            else:
                expected = path.read_text()
                actual = stable_manifest(manifest)
                if expected != actual:
                    print(f"  golden manifest mismatch: {path}")
                    if not check.optional:
                        failures.append(check.name + ":golden-mismatch")
    if failures:
        print("EMULATOR_ABI_SMOKE status=failed failures=" + ",".join(failures))
        return 1
    print("EMULATOR_ABI_SMOKE status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
