#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
IR = ROOT / "shader_recompiler" / "ir"
JSON_OUT = ROOT / "shader_recompiler_ir_core_parity_matrix.json"
MD_OUT = ROOT / "shader_recompiler_ir_core_parity_matrix.md"

STATUSES = ["MATCHED", "MISSING_IN_ELISA"]

@dataclass
class Symbol:
    name: str
    cpp_file: str
    elisa_expect: str


def parse_ir_emitter_methods() -> list[str]:
    cpp = (IR / "ir_emitter.cpp").read_text()
    names = re.findall(r"\bIREmitter::([A-Za-z_][A-Za-z0-9_]*)\s*\(", cpp)
    return names


def parse_block_methods() -> list[str]:
    h = (IR / "basic_block.h").read_text()
    return re.findall(r"\bvoid\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", h) + re.findall(
        r"\biterator\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", h
    )


def parse_program_symbols() -> list[str]:
    h = (IR / "program.h").read_text()
    out = re.findall(r"\bvoid\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", h)
    return out


def parse_simple_free_functions(path: Path) -> list[str]:
    text = path.read_text()
    return re.findall(r"\b(?:std::string|std::string_view|bool|BlockList|void)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", text)


def collect_required() -> list[Symbol]:
    req: list[Symbol] = []

    req.extend(
        [
            Symbol("Type::NameOf", "shader_recompiler/ir/type.cpp", "ShaderIrType_NameOf"),
            Symbol(
                "Type::AreTypesCompatible",
                "shader_recompiler/ir/type.cpp",
                "ShaderIrType_AreTypesCompatible",
            ),
            Symbol("Attribute::NameOf", "shader_recompiler/ir/attribute.cpp", "ShaderIrAttribute_NameOf"),
            Symbol("Patch::NameOf", "shader_recompiler/ir/patch.cpp", "ShaderIrPatch_NameOf"),
            Symbol("Opcode::NameOf", "shader_recompiler/ir/opcodes.cpp", "ShaderIrOpcode_NameOf"),
            Symbol("Opcode::TypeOf", "shader_recompiler/ir/opcodes.h", "ShaderIrOpcode_TypeOf"),
            Symbol("Opcode::NumArgsOf", "shader_recompiler/ir/opcodes.h", "ShaderIrOpcode_NumArgsOf"),
            Symbol("Opcode::ArgTypeOf", "shader_recompiler/ir/opcodes.h", "ShaderIrOpcode_ArgTypeOf"),
            Symbol("ASL::DumpASLNode", "shader_recompiler/ir/abstract_syntax_list.cpp", "ShaderIrDumpASLNode"),
            Symbol("PostOrder", "shader_recompiler/ir/post_order.cpp", "ShaderIrPostOrder"),
            Symbol("DumpProgram", "shader_recompiler/ir/program.cpp", "ShaderIrProgram_Dump"),
            Symbol("Inst::MayHaveSideEffects", "shader_recompiler/ir/microinstruction.cpp", "ShaderIrInst_MayHaveSideEffects"),
            Symbol("Inst::ReplaceUsesWith", "shader_recompiler/ir/microinstruction.cpp", "ShaderIrInst_ReplaceUsesWith"),
            Symbol("Inst::ReplaceUsesWithAndRemove", "shader_recompiler/ir/value.h", "ShaderIrInst_ReplaceUsesWithAndRemove"),
        ]
    )

    for name in sorted(set(parse_ir_emitter_methods())):
        req.append(
            Symbol(
                f"IREmitter::{name}",
                "shader_recompiler/ir/ir_emitter.cpp",
                f"ShaderIrEmitter_{name}",
            )
        )

    for name in sorted(set(parse_block_methods())):
        req.append(
            Symbol(
                f"Block::{name}",
                "shader_recompiler/ir/basic_block.h",
                f"ShaderIrBlock_{name}",
            )
        )

    for name in sorted(set(parse_program_symbols())):
        req.append(
            Symbol(
                f"Program::{name}",
                "shader_recompiler/ir/program.h",
                f"ShaderIrProgram_{name}",
            )
        )

    # Known core helpers from value.cpp and value.h
    for name in [
        "ShaderIrValue_FromInst",
        "ShaderIrValue_FromU1",
        "ShaderIrValue_FromU8",
        "ShaderIrValue_FromU16",
        "ShaderIrValue_FromU32",
        "ShaderIrValue_FromF32",
        "ShaderIrValue_FromU64",
        "ShaderIrValue_FromF64",
        "ShaderIrValue_Type",
        "ShaderIrValue_IsImmediate",
        "ShaderIrValue_Equals",
        "ShaderIrInst_SetArg",
        "ShaderIrInst_PhiBlock",
        "ShaderIrInst_AddPhiOperand",
        "ShaderIrInst_Invalidate",
        "ShaderIrInst_ClearArgs",
        "ShaderIrInst_ReplaceOpcode",
        "ShaderIrInst_AreAllArgsImmediates",
    ]:
        req.append(Symbol(name, "shader_recompiler/ir/value.{h,cpp}", name))

    # Deduplicate by symbol + expected name
    uniq: dict[tuple[str, str], Symbol] = {}
    for s in req:
        uniq[(s.name, s.elisa_expect)] = s
    return sorted(uniq.values(), key=lambda s: s.name)


def parse_elisa_functions() -> set[str]:
    funcs: set[str] = set()
    for path in sorted(IR.glob("*.elisa")):
        text = path.read_text()
        for m in re.finditer(r"^def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(", text, re.M):
            funcs.add(m.group(1))
    return funcs


def has_match(funcs: set[str], expect: str) -> bool:
    if expect in funcs:
        return True
    # emitter overloads are suffixed with _2, _3, ...
    if expect.startswith("ShaderIrEmitter_"):
        prefix = expect + "_"
        return any(fn.startswith(prefix) for fn in funcs)
    return False


def generate() -> dict:
    required = collect_required()
    funcs = parse_elisa_functions()

    entries = []
    counts = {k: 0 for k in STATUSES}
    for symbol in required:
        matched = has_match(funcs, symbol.elisa_expect)
        status = "MATCHED" if matched else "MISSING_IN_ELISA"
        counts[status] += 1
        entries.append(
            {
                "symbol": symbol.name,
                "status": status,
                "cpp_file": symbol.cpp_file,
                "elisa_symbol": symbol.elisa_expect,
            }
        )

    return {
        "scope": "shader_recompiler/ir core",
        "status_values": STATUSES,
        "counts": counts,
        "entries": entries,
    }


def write_markdown(data: dict) -> str:
    lines: list[str] = []
    lines.append("# Shader Recompiler IR Core Parity Matrix")
    lines.append("")
    total = sum(int(v) for v in data["counts"].values())
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- Total symbols: `{total}`")
    for key in STATUSES:
        lines.append(f"- {key}: `{data['counts'][key]}`")
    lines.append("")
    lines.append("## Entries")
    lines.append("")
    lines.append("| Symbol | Status | C++ | Elisa |")
    lines.append("|---|---|---|---|")
    for e in data["entries"]:
        lines.append(
            f"| `{e['symbol']}` | `{e['status']}` | `{e['cpp_file']}` | `{e['elisa_symbol']}` |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    data = generate()
    JSON_OUT.write_text(json.dumps(data, indent=2) + "\n")
    MD_OUT.write_text(write_markdown(data))
    print(f"wrote {JSON_OUT.relative_to(ROOT)}")
    print(f"wrote {MD_OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
