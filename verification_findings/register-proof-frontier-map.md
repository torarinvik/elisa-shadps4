# Register Proof Frontier Map

Audit base: integrated head `098ff2d6fb2ff1c4039440bc798ca8f056d9a315`.

## Current Proof-Carrying Flow

The proven register-index path now exists from GCN decode through translator selection and into IR value construction:

1. Decode-side register facts are introduced in `shader_recompiler/frontend/instruction.elisa`.
   - `GcnScalarRegisterIndex = u32 is InRange[0, 127]`
   - `GcnVectorRegisterIndex = u32 is InRange[0, 255]`
   - `InstOperand_SetScalarRegisterCode`
   - `InstOperand_SetVectorRegisterCode`
   - `InstOperand_RefinedScalarRegister`
   - `InstOperand_RefinedVectorRegister`
   - `GcnScalarRegisterIndex_Try`
   - `GcnVectorRegisterIndex_Try`
   - `InstOperand_TryVectorRegisterIndex`
2. Proven field extractors feed those types in `shader_recompiler/frontend/gcn_decode_proven.elisa`, with worked smaller examples in:
   - `shader_recompiler/frontend/std_decode.elisa`
   - `shader_recompiler/frontend/sop2_field_decode_proven.elisa`
   - `shader_recompiler/frontend/vop2_field_decode_proven.elisa`
3. Real decode copies refined fields into `InstOperand` in `shader_recompiler/frontend/decode.elisa`.
   - Representative refined assignments: `vdst: GcnVectorRegisterIndex = d.vdst`, `sbase: GcnScalarRegisterIndex = d.sbase`, `srsrc: GcnScalarRegisterIndex = d.srsrc`, `ssamp: GcnScalarRegisterIndex = d.ssamp`.
4. Translator operand selection prefers refined operand metadata before falling back to the raw encoded code in `shader_recompiler/frontend/translate/translate.elisa`.
   - `vgpr_reg_index`
   - `sgpr_reg_index`
   - `scalar_operand_selector`
   - `vector_operand_selector`
   - raw extern bridge points: `operand_has_refined_sgpr`, `operand_refined_sgpr`, `operand_has_refined_vgpr`, `operand_refined_vgpr`, `operand_has_refined_scalar_selector`, `operand_refined_scalar_selector`, `operand_has_refined_vector_selector`, `operand_refined_vector_selector`
5. IR-side register file bounds are represented separately in `shader_recompiler/ir/reg.elisa`.
   - `IR_NUM_SCALAR_REGS = 104`
   - `IR_NUM_VECTOR_REGS = 256`
   - `ShaderIrScalarRegIndex = u32 is ShaderIrRegInRange[0, 103]`
   - `ShaderIrVectorRegIndex = u32 is ShaderIrRegInRange[0, 255]`
   - checked narrowing: `ShaderIrScalarRegIndex_TryFromU32`, `ShaderIrVectorRegIndex_TryFromU32`
6. IR values carry refined register indices through typed constructors and recover them through checked readers in `shader_recompiler/ir/value.elisa`.
   - `ShaderIrValue_FromScalarRegIndex`
   - `ShaderIrValue_FromVectorRegIndex`
   - `ShaderIrValue_TryScalarRegIndex`
   - `ShaderIrValue_TryVectorRegIndex`
   - `ShaderIrValue_ScalarRegIndex`
   - `ShaderIrValue_VectorRegIndex`
7. IR emitter typed entry points in `shader_recompiler/ir/ir_emitter.elisa` are the preferred proof-preserving API.
   - `ShaderIrEmitter_GetScalarRegIndex`
   - `ShaderIrEmitter_GetScalarRegIndexF32`
   - `ShaderIrEmitter_SetScalarRegIndex`
   - `ShaderIrEmitter_SetScalarRegIndexF32`
   - `ShaderIrEmitter_GetVectorRegIndex`
   - `ShaderIrEmitter_GetVectorRegIndexF32`
   - `ShaderIrEmitter_SetVectorRegIndex`
   - `ShaderIrEmitter_SetVectorRegIndexF32`

## Known Erasure And Check Points

The remaining frontier is not "can we prove a decoded register index?" It is "where is that proof erased, checked, or allowed to bypass the typed API?"

| Point | File/function | Status | Bounds-check implication |
| --- | --- | --- | --- |
| Decode field extraction | `shader_recompiler/frontend/gcn_decode_proven.elisa` and `decode.elisa` | Proven | Register field facts are available and should not need runtime array checks for decode-local SGPR/VGPR file access. |
| Operand metadata bridge | `shader_recompiler/frontend/translate/translate.elisa::{vgpr_reg_index,sgpr_reg_index}` | Proof preferred, raw fallback retained | Refined metadata wins, but fallback `operand_code` remains a compatibility path and must not be treated as proof-carrying. |
| Translator to C++/IR shim | `translate.elisa` externs `ir_get_*`, `ir_set_*`, `operand_*` | Opaque/raw `u32` ABI | Proof is erased at the raw extern boundary. Bounds-check elimination must be justified before this boundary or re-established after it. |
| Decode SGPR range vs IR SGPR range | `GcnScalarRegisterIndex[0,127]` to `ShaderIrScalarRegIndex[0,103]` | Requires narrowing | SGPR decode proof is wider than the IR scalar register file. Any path into typed IR scalar APIs must pass `ShaderIrScalarRegIndex_TryFromU32` or equivalent semantic filtering. |
| Decode VGPR range vs IR VGPR range | `GcnVectorRegisterIndex[0,255]` to `ShaderIrVectorRegIndex[0,255]` | Directly compatible | VGPR proof width matches IR vector register file, so the main risk is erasure, not range mismatch. |
| Legacy IR value constructors | `ShaderIrValue_FromScalarReg`, `ShaderIrValue_FromVectorReg` | Raw `u32` accepted | These are compatibility edges. New proof-carrying code should prefer `ShaderIrValue_FromScalarRegIndex` and `ShaderIrValue_FromVectorRegIndex`. |
| IR typed readers | `ShaderIrValue_TryScalarRegIndex`, `ShaderIrValue_TryVectorRegIndex` | Checked recovery | Good enforcement points when receiving an existing `ShaderIrValue` from mixed raw/typed code. |
| IR panic readers | `ShaderIrValue_ScalarRegIndex`, `ShaderIrValue_VectorRegIndex` | Assert-backed recovery | Useful internally only after proof/check. They should not become the first check after an opaque boundary. |
| IR emitter raw register APIs | `ShaderIrEmitter_GetScalarReg`, `ShaderIrEmitter_SetScalarReg`, `ShaderIrEmitter_GetVectorReg`, `ShaderIrEmitter_SetVectorReg` | Raw `ShaderIrValue` accepted | Keep for compatibility, but do not count as proof-preserving unless the `ShaderIrValue` came from a typed constructor or checked reader. |
| IR emitter typed register APIs | `ShaderIrEmitter_*RegIndex*` | Proof-preserving | Best enforcement target for API-hardening work. |

## Audit Commands

Run from the repo root:

```sh
rg -n "GcnScalarRegisterIndex|GcnVectorRegisterIndex|InstOperand_Set.*RegisterCode|InstOperand_Refined|Gcn.*RegisterIndex_Try|InstOperand_TryVectorRegisterIndex" shader_recompiler/frontend elisa_tests
rg -n "operand_has_refined|operand_refined|vgpr_reg_index|sgpr_reg_index|scalar_operand_selector|vector_operand_selector" shader_recompiler/frontend/translate
rg -n "ShaderIrScalarRegIndex|ShaderIrVectorRegIndex|TryFromU32|From.*RegIndex|Try.*RegIndex|ShaderIrEmitter_.*RegIndex" shader_recompiler/ir elisa_tests
rg -n "FromScalarReg\\(|FromVectorReg\\(|GetScalarReg\\(|SetScalarReg\\(|GetVectorReg\\(|SetVectorReg\\(" shader_recompiler elisa_tests
```

Lightweight validation targets for this frontier:

```sh
elisacore test shader-recompiler-ir-reg-tests --project .
elisacore test shader-recompiler-ir-core-pure-tests --project .
elisacore test shader-recompiler-frontend-instruction-index-tests --project .
python3 shader_recompiler_ir_core_parity_matrix_check.py
```

If `elisacore` is not on `PATH`, use the local compiler binary used elsewhere in this repo:

```sh
../compiler/bin/elisacore test shader-recompiler-ir-reg-tests --project .
```

## Recommended Merge Order

1. Land lowered-output and regression-test audit branches first. They define the observable before/after for check removal.
2. Land IR API enforcement next. Typed `ShaderIr*RegIndex` entry points and checked recovery must be the stable contract before production lowering removes checks.
3. Land translator hot-path conversion after IR enforcement. Convert call sites to typed narrowing at the latest proof-preserving point, especially SGPR paths where `[0,127]` must narrow to `[0,103]`.
4. Land compiler/codegen bounds-check elimination after the code has explicit typed/checking boundaries. The compiler pass should only erase checks that are discharged by refinements or by typed IR APIs, not by raw extern conventions.
5. Land backend/lowered-output cleanup last, then rerun the audit commands above and compare generated lowered output for actual check removal plus no raw bypass regressions.

## Acceptance Bar For Bounds-Check Elimination

Do not remove a register bounds check merely because the value originated in decode. A removal is safe only when the reviewed path is one of:

- decode-local access with a `Gcn*RegisterIndex` refinement and matching register-file size;
- VGPR decode proof carried directly into `ShaderIrVectorRegIndex`;
- SGPR decode proof narrowed through `ShaderIrScalarRegIndex_TryFromU32` or a typed equivalent;
- existing `ShaderIrValue` recovered through `ShaderIrValue_Try*RegIndex` before calling typed IR emitter APIs.

Everything crossing `Unsafe.RawExtern` as `u32` should be treated as proof-erased until rechecked.
