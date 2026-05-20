#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
JSON_PATH = ROOT / "shader_recompiler_ir_core_parity_matrix.json"
IR_EMITTER_PATH = ROOT / "shader_recompiler" / "ir" / "ir_emitter.elisa"


def emitter_behavior_audit() -> tuple[list[str], list[str]]:
    if not IR_EMITTER_PATH.exists():
        return (["missing ir emitter file"], [])

    text = IR_EMITTER_PATH.read_text()
    syntax_issues: list[str] = []
    # Catch broken generated C++ ternary/path remnants in Elisa.
    if "OPCODE::" in text:
        syntax_issues.append("contains invalid token 'OPCODE::'")
    if re.search(r"IR_OPCODE_[A-Z0-9_]+\s*:\s*[A-Z_]+::", text):
        syntax_issues.append("contains invalid generated opcode selection expression")

    placeholder_funcs: list[str] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = re.match(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", lines[i])
        if not m:
            i += 1
            continue
        name = m.group(1)
        j = i + 1
        body: list[str] = []
        while j < len(lines) and not lines[j].startswith("def "):
            body.append(lines[j])
            j += 1
        body_text = "\n".join(body)

        # Flag only true placeholders: empty return with discarded args and no emission/dispatch.
        if (
            "return ShaderIrValue_Empty()" in body_text
            and re.search(r"^\s*_\s*=", body_text, re.M)
            and "ShaderIrEmitter_Emit(" not in body_text
            and name != "ShaderIrEmitter_Emit"
        ):
            placeholder_funcs.append(name)
        i = j

    return (syntax_issues, placeholder_funcs)


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

    syntax_issues, placeholder_funcs = emitter_behavior_audit()

    if args.summary:
        total = sum(int(v) for v in counts.values())
        print(f"shader_recompiler/ir core parity: total={total} matched={counts.get('MATCHED', 0)} missing={missing}")
        print(
            f"shader_recompiler/ir emitter behavior audit: syntax_issues={len(syntax_issues)} placeholders={len(placeholder_funcs)}"
        )

    if missing != 0:
        print("IR core parity matrix has missing Elisa symbols")
        return 1
    if syntax_issues:
        print("IR emitter behavior audit failed:")
        for issue in syntax_issues:
            print(f"- {issue}")
        return 1
    if placeholder_funcs:
        print("IR emitter behavior audit found placeholder functions:")
        for fn in placeholder_funcs:
            print(f"- {fn}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
