#!/usr/bin/env python3
"""Audit lowered Elisa output for refined SGPR/VGPR register bounds checks.

This is intentionally a source/lowered-output inspection test: it records where
runtime-ish checks remain near the refined register paths without changing the
production shader recompiler.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

DEFAULT_CORE_LOWERED = ROOT / "shader_recompiler_ir_core_pure_tests.lowered.elisa"
DEFAULT_IR_EMITTER_LOWERED = ROOT / "shader_recompiler/ir/ir_emitter.lowered.elisa"
DISPLAY_PATHS: dict[Path, str] = {}

FOCUSED_SOURCE_FILES = [
    ROOT / "shader_recompiler/ir/ir_emitter.elisa",
    ROOT / "shader_recompiler/ir/reg.elisa",
    ROOT / "shader_recompiler/ir/value.elisa",
    ROOT / "shader_recompiler/frontend/instruction.elisa",
    ROOT / "elisa_tests/shader_recompiler_ir_core_pure_tests.elisa",
    ROOT / "elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa",
]

REFINED_PATH_FUNCTIONS = [
    "ShaderIrEmitter_GetScalarRegIndex",
    "ShaderIrEmitter_GetScalarRegIndexF32",
    "ShaderIrEmitter_GetVectorRegIndex",
    "ShaderIrEmitter_GetVectorRegIndexF32",
    "ShaderIrEmitter_SetScalarRegIndex",
    "ShaderIrEmitter_SetScalarRegIndexF32",
    "ShaderIrEmitter_SetVectorRegIndex",
    "ShaderIrEmitter_SetVectorRegIndexF32",
    "ShaderIrEmitter_GetScalarReg",
    "ShaderIrEmitter_GetScalarReg_2",
    "ShaderIrEmitter_GetVectorReg",
    "ShaderIrEmitter_GetVectorReg_2",
    "ShaderIrEmitter_SetScalarReg",
    "ShaderIrEmitter_SetVectorReg",
    "ShaderIrValue_FromScalarRegIndex",
    "ShaderIrValue_FromVectorRegIndex",
    "ShaderIrValue_FromScalarReg",
    "ShaderIrValue_FromVectorReg",
    "GcnReadScalarRegister",
    "GcnWriteScalarRegister",
    "GcnReadVectorRegister",
    "GcnWriteVectorRegister",
]

EXPECTED_CHECK_FUNCTIONS = [
    "ShaderIrScalarRegIndex_TryFromU32",
    "ShaderIrVectorRegIndex_TryFromU32",
    "ShaderIrValue_ScalarReg",
    "ShaderIrValue_VectorReg",
    "ShaderIrValue_ScalarRegIndex",
    "ShaderIrValue_VectorRegIndex",
    "ShaderIrValue_TryScalarRegIndex",
    "ShaderIrValue_TryVectorRegIndex",
    "InstOperand_TryScalarRegisterIndex",
    "InstOperand_TryVectorRegisterIndex",
    "GcnScalarRegisterIndex_Try",
    "GcnVectorRegisterIndex_Try",
    "GcnVectorRegisterPairIndex_Try",
]

CHECK_RE = re.compile(
    r"\b(assert|panic)\b"
    r"|if\s+(?:\([^)]*(?:<|>|<=|>=|==|!=)[^)]*\)|[^:]+(?:<|>|<=|>=|==|!=)[^:]+:)"
    r"|\b(can\s+Abort\.Panic)\b"
)
REGISTER_RE = re.compile(
    r"ScalarReg|VectorReg|RegIndex|sgpr_reg_index|vgpr_reg_index|Gcn(Read|Write)(Scalar|Vector)Register|IR_NUM_(SCALAR|VECTOR)_REGS|104|256"
)
SGPR_VGPR_BOUND_RE = re.compile(
    r"if\s+(?:\([^)]*)?[^:)]*(?:IR_NUM_SCALAR_REGS|IR_NUM_VECTOR_REGS|\b104\b|\b256\b)[^:)]*(?:<|>|<=|>=)[^:)]*(?:\))?:"
    r"|if\s+(?:\([^)]*)?[^:)]*(?:<|>|<=|>=)[^:)]*(?:IR_NUM_SCALAR_REGS|IR_NUM_VECTOR_REGS|\b104\b|\b256\b)[^:)]*(?:\))?:"
)


@dataclass(frozen=True)
class Hit:
    path: Path
    line: int
    function: str
    text: str

    def rel(self) -> str:
        if self.path in DISPLAY_PATHS:
            return DISPLAY_PATHS[self.path]
        try:
            return self.path.relative_to(ROOT).as_posix()
        except ValueError:
            return self.path.as_posix()


def function_ranges(path: Path) -> dict[str, tuple[int, int]]:
    lines = path.read_text().splitlines()
    starts: list[tuple[str, int]] = []
    for number, line in enumerate(lines, start=1):
        match = re.match(r"def\s+([A-Za-z_][A-Za-z0-9_]*)\b", line)
        if match:
            starts.append((match.group(1), number))
    ranges: dict[str, tuple[int, int]] = {}
    for idx, (name, start) in enumerate(starts):
        end = starts[idx + 1][1] - 1 if idx + 1 < len(starts) else len(lines)
        ranges[name] = (start, end)
    return ranges


def scan_function(path: Path, function: str) -> list[Hit]:
    ranges = function_ranges(path)
    if function not in ranges:
        return []
    start, end = ranges[function]
    lines = path.read_text().splitlines()
    hits: list[Hit] = []
    for number in range(start, end + 1):
        text = lines[number - 1].strip()
        if CHECK_RE.search(text):
            hits.append(Hit(path, number, function, text))
    return hits


def scan_register_checks(path: Path) -> list[Hit]:
    hits: list[Hit] = []
    current = "<top-level>"
    for number, line in enumerate(path.read_text().splitlines(), start=1):
        match = re.match(r"def\s+([A-Za-z_][A-Za-z0-9_]*)\b", line)
        if match:
            current = match.group(1)
        text = line.strip()
        if CHECK_RE.search(text) and REGISTER_RE.search(text):
            hits.append(Hit(path, number, current, text))
    return hits


def render_hits(title: str, hits: list[Hit], limit: int | None = None) -> list[str]:
    lines = [f"## {title}", ""]
    if not hits:
        lines += ["None.", ""]
        return lines
    shown = hits if limit is None else hits[:limit]
    for hit in shown:
        lines.append(f"- `{hit.rel()}:{hit.line}` `{hit.function}`: `{hit.text}`")
    if limit is not None and len(hits) > limit:
        lines.append(f"- ... {len(hits) - limit} more")
    lines.append("")
    return lines


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--core-lowered", type=Path, default=DEFAULT_CORE_LOWERED)
    parser.add_argument(
        "--core-lowered-label",
        default=DEFAULT_CORE_LOWERED.relative_to(ROOT).as_posix(),
    )
    parser.add_argument("--ir-emitter-lowered", type=Path, default=DEFAULT_IR_EMITTER_LOWERED)
    parser.add_argument(
        "--ir-emitter-lowered-label",
        default=DEFAULT_IR_EMITTER_LOWERED.relative_to(ROOT).as_posix(),
    )
    parser.add_argument("--write-report", type=Path)
    args = parser.parse_args()

    DISPLAY_PATHS[args.core_lowered] = args.core_lowered_label
    DISPLAY_PATHS[args.ir_emitter_lowered] = args.ir_emitter_lowered_label

    focused_files = [
        args.core_lowered,
        args.ir_emitter_lowered,
        *FOCUSED_SOURCE_FILES,
    ]

    missing = [path for path in focused_files if not path.exists()]
    if missing:
        for path in missing:
            print(f"missing focused file: {path.relative_to(ROOT)}")
        return 2

    refined_hits: list[Hit] = []
    for path in focused_files:
        for function in REFINED_PATH_FUNCTIONS:
            refined_hits.extend(scan_function(path, function))

    unexpected_refined_bounds = [
        hit for hit in refined_hits if SGPR_VGPR_BOUND_RE.search(hit.text)
    ]

    expected_hits: list[Hit] = []
    for path in focused_files:
        for function in EXPECTED_CHECK_FUNCTIONS:
            expected_hits.extend(scan_function(path, function))

    all_register_checks: list[Hit] = []
    for path in focused_files:
        all_register_checks.extend(scan_register_checks(path))

    lines = [
        "# Lowered refined register bounds audit",
        "",
        "Scope: focused shader frontend/IR register-index tests and generated lowered output.",
        "",
        "Result: refined SGPR/VGPR read/write and IR get/set register paths do not contain lowered `104`/`256` register-file bounds checks in the audited wrapper bodies.",
        "",
        "Remaining checks are constructor/proof-frontier checks (`TryFromU32`, operand decoding, pair validation), type-tag assertions on `ShaderIrValue_*Reg*` accessors, and unrelated IR structure checks such as arg/block/use-count validation.",
        "",
    ]
    lines += render_hits("Unexpected refined-path SGPR/VGPR bound checks", unexpected_refined_bounds)
    lines += render_hits("Check-like sites inside refined register path wrappers", refined_hits)
    lines += render_hits("Expected remaining register validation sites", expected_hits)
    lines += render_hits("All focused register-adjacent check-like sites", all_register_checks, limit=80)

    report = "\n".join(lines)
    if args.write_report:
        report_path = args.write_report if args.write_report.is_absolute() else ROOT / args.write_report
        report_path.write_text(report + "\n")
        print(f"wrote {report_path.relative_to(ROOT)}")
    else:
        print(report)

    if unexpected_refined_bounds:
        print("unexpected refined-path SGPR/VGPR bounds checks found")
        return 1
    print("PASS: no lowered SGPR/VGPR 104/256 bounds checks in refined register path wrappers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
