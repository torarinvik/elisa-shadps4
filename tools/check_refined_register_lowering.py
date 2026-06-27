#!/usr/bin/env python3
"""Check refined register regression coverage in generated Elisa lowering.

The pure Elisa tests prove the executable boundary behavior. This script adds a
lightweight lowered-output guard: after the raw TryFromU32 boundary narrows the
max valid SGPR/VGPR, the emitted-register assertion segment must not contain a
new raw register-file bounds check.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


TEST_NAME = "shader_ir_refined_register_index_boundary_elides_recheck_regression"
SEGMENT_START = "ShaderIrEmitter_GetScalarRegIndex"
SEGMENT_END = "block: ShaderIrBlock"
FORBIDDEN_AFTER_BOUNDARY = (
    "ShaderIrScalarRegIndex_TryFromU32",
    "ShaderIrVectorRegIndex_TryFromU32",
    "IR_NUM_SCALAR_REGS",
    "IR_NUM_VECTOR_REGS",
    'panic("assert',
)


def extract_function(text: str, name: str) -> str:
    match = re.search(rf"^def {re.escape(name)}\b.*?(?=^def |\Z)", text, re.M | re.S)
    if match is None:
        raise SystemExit(f"missing lowered test function: {name}")
    return match.group(0)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "lowered",
        nargs="?",
        default="shader_recompiler_ir_core_pure_tests.lowered.elisa",
        type=Path,
    )
    args = parser.parse_args()

    lowered = args.lowered.read_text(encoding="utf-8")
    body = extract_function(lowered, TEST_NAME)

    for needle in (
        "ShaderIrScalarRegIndex_TryFromU32(103",
        "ShaderIrVectorRegIndex_TryFromU32(255",
        "not ShaderIrScalarRegIndex_TryFromU32(104",
        "not ShaderIrVectorRegIndex_TryFromU32(256",
        "IR_OPCODE_GETSCALARREGISTER",
        "IR_OPCODE_SETSCALARREGISTER",
        "IR_OPCODE_GETVECTORREGISTER",
        "IR_OPCODE_SETVECTORREGISTER",
    ):
        if needle not in body:
            raise SystemExit(f"lowered regression body missing expected proof: {needle}")

    start = body.find(SEGMENT_START)
    end = body.find(SEGMENT_END, start)
    if start < 0 or end < 0:
        raise SystemExit("could not isolate refined emission segment in lowered test")
    emit_segment = body[start:end]

    for forbidden in FORBIDDEN_AFTER_BOUNDARY:
        if forbidden in emit_segment:
            raise SystemExit(f"refined emission segment contains recheck marker: {forbidden}")

    print("refined register lowered check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
