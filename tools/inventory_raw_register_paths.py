#!/usr/bin/env python3
"""Inventory raw SGPR/VGPR index/value paths in the shader frontier."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class Pattern:
    label: str
    classification: str
    path: str
    regex: str


PATTERNS = (
    Pattern(
        "IR raw value constructors/accessors",
        "compatibility adapter",
        "shader_recompiler/ir/value.elisa",
        r"def ShaderIrValue_(?:FromScalarReg|FromVectorReg|ScalarReg|VectorReg)\(",
    ),
    Pattern(
        "IR refined/raw conversion boundary",
        "external decode boundary",
        "shader_recompiler/ir/value.elisa",
        r"def ShaderIrValue_(?:TryScalarRegIndex|TryVectorRegIndex|ScalarRegIndex|VectorRegIndex)\(",
    ),
    Pattern(
        "IR raw emitter register adapters",
        "compatibility adapter",
        "shader_recompiler/ir/ir_emitter.elisa",
        r"def ShaderIrEmitter_(?:GetScalarReg|GetScalarReg_2|GetScalarRegF32|GetVectorReg|GetVectorReg_2|GetVectorRegF32|SetScalarReg|SetVectorReg)\(",
    ),
    Pattern(
        "IR refined emitter wrappers",
        "external decode boundary",
        "shader_recompiler/ir/ir_emitter.elisa",
        r"def ShaderIrEmitter_(?:GetScalarRegIndex|GetScalarRegIndexF32|GetVectorRegIndex|GetVectorRegIndexF32|SetScalarRegIndex|SetScalarRegIndexF32|SetVectorRegIndex|SetVectorRegIndexF32)\(",
    ),
    Pattern(
        "Frontend operand raw code storage/extraction",
        "external decode boundary",
        "shader_recompiler/frontend/instruction.elisa",
        r"(?:op\.code <-|return op\.code|out\.value <- op\.code|def InstOperand_Try(?:Scalar|Vector)RegisterIndex\()",
    ),
    Pattern(
        "Frontend register-file direct read/write helpers",
        "external decode boundary",
        "shader_recompiler/frontend/instruction.elisa",
        r"def Gcn(?:Read|Write)(?:Scalar|Vector)Register\(",
    ),
    Pattern(
        "Translator raw extern register emitter ABI",
        "external decode boundary",
        "shader_recompiler/frontend/translate/translate.elisa",
        r"extern ir_(?:get|set)_(?:scalar_reg|thread_bit_scalar_reg|vector_reg_[uf]32)\(",
    ),
    Pattern(
        "Translator compat raw index adapters",
        "removable unsafe path",
        "shader_recompiler/frontend/translate/translate.elisa",
        r"def (?:compat_[sv]gpr_reg_index|descriptor_sgpr_lane|(?:get|set)_(?:thread_bit_)?sgpr|(?:get|set)_vgpr_[uf]32|(?:[sv]gpr|sgpr_resource|thread_bit_sgpr|set_[sv]gpr_[uf]32)_at)\(",
    ),
    Pattern(
        "Frontend fetch-shader raw operand reads",
        "compatibility adapter",
        "shader_recompiler/frontend/fetch_shader.elisa",
        r"(?:dest_vgpr|sgpr_base|vertex_offset_sgpr|instance_offset_sgpr|base_sgpr|\.code)",
    ),
    Pattern(
        "Raw compatibility regression tests",
        "test-only coverage",
        "elisa_tests/shader_recompiler_ir_core_pure_tests.elisa",
        r"(?:ShaderIrValue_From(?:Scalar|Vector)Reg\(|ShaderIrEmitter_(?:GetScalarReg|SetVectorReg)\(|== (?:104|256)|not ShaderIrValue_Try(?:Scalar|Vector)RegIndex)",
    ),
    Pattern(
        "Frontend raw/refined instruction tests",
        "test-only coverage",
        "elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa",
        r"(?:\.code <-|Gcn(?:Read|Write)(?:Scalar|Vector)Register|InstOperand_Try(?:Scalar|Vector)RegisterIndex|sgprs\[|vgprs\[)",
    ),
)


def matches(pattern: Pattern) -> list[tuple[int, str]]:
    path = ROOT / pattern.path
    text = path.read_text(encoding="utf-8")
    out: list[tuple[int, str]] = []
    rx = re.compile(pattern.regex)
    for lineno, line in enumerate(text.splitlines(), 1):
        if rx.search(line):
            out.append((lineno, line.strip()))
    return out


def main() -> int:
    print("# Raw register path inventory\n")
    by_class: dict[str, int] = {}
    failed = False
    for pattern in PATTERNS:
        found = matches(pattern)
        by_class[pattern.classification] = by_class.get(pattern.classification, 0) + len(found)
        print(f"## {pattern.label}")
        print(f"classification: {pattern.classification}")
        print(f"path: {pattern.path}")
        print(f"matches: {len(found)}")
        if not found:
            failed = True
            print("ERROR: no matches")
        for lineno, line in found:
            print(f"- {pattern.path}:{lineno}: {line}")
        print()

    print("## Totals")
    for classification in sorted(by_class):
        print(f"- {classification}: {by_class[classification]}")

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
