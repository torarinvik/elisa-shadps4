# Raw SGPR/VGPR path inventory

Scope: shader recompiler IR/frontend register-index frontier on branch `codex/frontier-raw-register-inventory`, based on `work`.

This inventory tracks paths that still carry raw SGPR/VGPR indices or raw register values after the refined-register work. It intentionally excludes unrelated `.value` fields and non-shader raw memory paths.

Audit command:

```sh
python3 tools/inventory_raw_register_paths.py
python3 tools/check_refined_register_lowering.py
python3 elisa_tests/audit_lowered_register_bounds.py
```

Note: `tools/check_refined_register_lowering.py` is an existing narrow guard and currently expects a lowered test function that is absent from the checked-in `shader_recompiler_ir_core_pure_tests.lowered.elisa` on this branch. The broader `elisa_tests/audit_lowered_register_bounds.py` audit passes and is the sanity check used for this report.

## Summary

| Classification | Count | Meaning |
| --- | ---: | --- |
| External decode boundary | 34 | Raw bits enter from instruction decode, checked lowering, or C++/extern translator ABI. Keep until the boundary type can be made refined end-to-end. |
| Compatibility adapter | 36 | Raw `u32` helpers retained so legacy translator/emitter code can compile while refined wrappers migrate call sites. |
| Removable unsafe path | 15 | Raw shim layer that can be removed once translator calls accept refined SGPR/VGPR types directly. |
| Test-only coverage | 27 | Tests that intentionally exercise raw compatibility behavior or boundary rejection. |

## External Decode Boundary

- `shader_recompiler/ir/value.elisa:203` `ShaderIrValue_TryScalarRegIndex` narrows a raw `ShaderIrValue.scalar_reg` payload to `ShaderIrScalarRegIndex`.
- `shader_recompiler/ir/value.elisa:209` `ShaderIrValue_ScalarRegIndex` is the panic-on-invalid extraction boundary for scalar values.
- `shader_recompiler/ir/value.elisa:221` `ShaderIrValue_TryVectorRegIndex` narrows a raw `ShaderIrValue.vector_reg` payload to `ShaderIrVectorRegIndex`.
- `shader_recompiler/ir/value.elisa:227` `ShaderIrValue_VectorRegIndex` is the panic-on-invalid extraction boundary for vector values.
- `shader_recompiler/ir/ir_emitter.elisa:203,217,230,244,263,274,290,301` refined emitter wrappers accept refined SGPR/VGPR aliases, then delegate to raw compatibility emitters.
- `shader_recompiler/frontend/instruction.elisa:210,216,222,228,237,239,374,378,382,386` keeps `InstOperand.code` as the decoded raw operand code, with `InstOperand_Try*RegisterIndex` as the narrowing boundary.
- `shader_recompiler/frontend/instruction.elisa:398,402,406,410` directly indexes the decoded SGPR/VGPR register files after receiving refined frontend indices.
- `shader_recompiler/frontend/translate/translate.elisa:64-74` exposes the raw extern C++ translator ABI: `ir_get_scalar_reg`, `ir_set_scalar_reg`, `ir_get_thread_bit_scalar_reg`, `ir_set_thread_bit_scalar_reg`, `ir_get_vector_reg_u32`, `ir_get_vector_reg_f32`, `ir_set_vector_reg_u32`, and `ir_set_vector_reg_f32`.

## Compatibility Adapter

- `shader_recompiler/ir/value.elisa:74` `ShaderIrValue_FromScalarReg(reg: u32)` can still construct an out-of-range scalar register value.
- `shader_recompiler/ir/value.elisa:91` `ShaderIrValue_FromVectorReg(reg: u32)` can still construct an out-of-range vector register value.
- `shader_recompiler/ir/value.elisa:198` `ShaderIrValue_ScalarReg` returns the raw scalar payload after only a type-tag assertion.
- `shader_recompiler/ir/value.elisa:216` `ShaderIrValue_VectorReg` returns the raw vector payload after only a type-tag assertion.
- `shader_recompiler/ir/ir_emitter.elisa:198,208,214,225,235,241,252,279` raw emitter adapters accept `ShaderIrValue` register operands and are intentionally named as compatibility shims.
- `shader_recompiler/frontend/fetch_shader.elisa:20,22,33,34,48,56,79,109,110,111,115,116,126,129,131,132,133,135` parses fetch-shader pseudo-instructions from raw operand codes into compact fetch metadata. This is not IR register-file access, but it is still a raw SGPR/VGPR value path.

## Removable Unsafe Path

- `shader_recompiler/frontend/translate/translate.elisa:75` `compat_vgpr_reg_index` returns `operand_code` when the operand lacks a refined VGPR.
- `shader_recompiler/frontend/translate/translate.elisa:80` `compat_sgpr_reg_index` returns `operand_code` when the operand lacks a refined SGPR.
- `shader_recompiler/frontend/translate/translate.elisa:111` `descriptor_sgpr_lane` multiplies a raw/compat SGPR by four and adds a lane.
- `shader_recompiler/frontend/translate/translate.elisa:117,121,125,129,133,137,141,145` raw `get_*`/`set_*` helpers pass `u32` registers into unsafe extern emitter calls.
- `shader_recompiler/frontend/translate/translate.elisa:157,161,165,189` offset helpers still build raw register indices from compatibility extraction.

These are the main next-removal candidates. The refined siblings at `translate.elisa:95-101` and `173-187` prove the intended shape, but the underlying extern ABI still consumes `u32`, so removal should happen together with typed extern/wrapper changes.

## Test-Only Coverage

- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:157-194` intentionally constructs SGPR 104 and VGPR 256 through raw value/emitter adapters and verifies refined extraction rejects them.
- `elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa:13-39` exercises raw operand code staging, frontend register-file read/write helpers, pair-index rejection, and wrong-field rejection.
- `elisa_tests/shader_recompiler_frontend_pure_tests.elisa:139-331` asserts decoded raw operand codes alongside `has_refined_sgpr`/`has_refined_vgpr` flags.
- `elisa_tests/shader_recompiler_ir_reg_tests.elisa:20-40` covers refined SGPR/VGPR `TryFromU32` boundary behavior.

## Suggested Next Removals

1. Replace translator `compat_sgpr_reg_index`/`compat_vgpr_reg_index` call sites with the existing `refined_*` helpers where decode already sets `has_refined_*`.
2. Add typed translator wrappers for `ir_get_*_reg`/`ir_set_*_reg` so raw `u32` is not the local Elisa-facing API, even if the final extern remains raw temporarily.
3. After translator migration, delete `ShaderIrEmitter_*ScalarReg`/`*VectorReg` raw adapters or move them behind test-only compatibility coverage.
4. Keep decode and fetch-shader raw paths until their source instruction fields are modeled as refined aliases at decode construction time.
