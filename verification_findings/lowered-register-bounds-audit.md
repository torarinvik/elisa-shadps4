# Lowered refined register bounds audit

Scope: focused shader frontend/IR register-index tests and checked-in lowered output at integrated head 098ff2d.

Result: refined SGPR/VGPR read/write and IR get/set register paths do not contain lowered `104`/`256` register-file bounds checks in the audited wrapper bodies.

Remaining checks are constructor/proof-frontier checks (`TryFromU32`, operand decoding, pair validation), type-tag assertions on `ShaderIrValue_*Reg*` accessors, and unrelated IR structure checks such as arg/block/use-count validation.

## Unexpected refined-path SGPR/VGPR bound checks

None.

## Check-like sites inside refined register path wrappers

- `shader_recompiler_ir_core_pure_tests.lowered.elisa:11696` `ShaderIrEmitter_SetScalarReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:11707` `ShaderIrEmitter_SetVectorReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:7376` `ShaderIrEmitter_SetScalarReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:7387` `ShaderIrEmitter_SetVectorReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.elisa:244` `ShaderIrEmitter_SetScalarReg`: `if value.type_bits == IR_TYPE_F32:`
- `shader_recompiler/ir/ir_emitter.elisa:263` `ShaderIrEmitter_SetVectorReg`: `if value.type_bits == IR_TYPE_F32:`

## Expected remaining register validation sites

- `shader_recompiler_ir_core_pure_tests.lowered.elisa:10766` `ShaderIrValue_ScalarReg`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:10770` `ShaderIrValue_VectorReg`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:6446` `ShaderIrValue_ScalarReg`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:6450` `ShaderIrValue_VectorReg`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler/ir/reg.elisa:30` `ShaderIrScalarRegIndex_TryFromU32`: `if reg < IR_NUM_SCALAR_REGS:`
- `shader_recompiler/ir/reg.elisa:36` `ShaderIrVectorRegIndex_TryFromU32`: `if reg < IR_NUM_VECTOR_REGS:`
- `shader_recompiler/ir/value.elisa:193` `ShaderIrValue_ScalarReg`: `assert value.type_bits == IR_TYPE_SCALARREG`
- `shader_recompiler/ir/value.elisa:211` `ShaderIrValue_VectorReg`: `assert value.type_bits == IR_TYPE_VECTORREG`
- `shader_recompiler/ir/value.elisa:204` `ShaderIrValue_ScalarRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/value.elisa:205` `ShaderIrValue_ScalarRegIndex`: `assert value.type_bits == IR_TYPE_SCALARREG`
- `shader_recompiler/ir/value.elisa:206` `ShaderIrValue_ScalarRegIndex`: `assert value.scalar_reg <= 103`
- `shader_recompiler/ir/value.elisa:222` `ShaderIrValue_VectorRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/value.elisa:223` `ShaderIrValue_VectorRegIndex`: `assert value.type_bits == IR_TYPE_VECTORREG`
- `shader_recompiler/ir/value.elisa:224` `ShaderIrValue_VectorRegIndex`: `assert value.vector_reg <= 255`
- `shader_recompiler/ir/value.elisa:198` `ShaderIrValue_TryScalarRegIndex`: `if value.type_bits != IR_TYPE_SCALARREG:`
- `shader_recompiler/ir/value.elisa:216` `ShaderIrValue_TryVectorRegIndex`: `if value.type_bits != IR_TYPE_VECTORREG:`
- `shader_recompiler/frontend/instruction.elisa:376` `InstOperand_TryScalarRegisterIndex`: `if op.field == OPERAND_FIELD_SCALAR_GPR and op.code <= OPERAND_FIELD_RANGE_SCALAR_GPR_MAX:`
- `shader_recompiler/frontend/instruction.elisa:384` `InstOperand_TryVectorRegisterIndex`: `if op.field == OPERAND_FIELD_VECTOR_GPR and op.code <= 255:`
- `shader_recompiler/frontend/instruction.elisa:328` `GcnScalarRegisterIndex_Try`: `if value <= 127:`
- `shader_recompiler/frontend/instruction.elisa:336` `GcnVectorRegisterIndex_Try`: `if value <= 255:`
- `shader_recompiler/frontend/instruction.elisa:352` `GcnVectorRegisterPairIndex_Try`: `if value <= 254:`

## All focused register-adjacent check-like sites

- `shader_recompiler/ir/reg.elisa:30` `ShaderIrScalarRegIndex_TryFromU32`: `if reg < IR_NUM_SCALAR_REGS:`
- `shader_recompiler/ir/reg.elisa:36` `ShaderIrVectorRegIndex_TryFromU32`: `if reg < IR_NUM_VECTOR_REGS:`
- `shader_recompiler/ir/reg.elisa:52` `ShaderIrScalarReg_Add`: `assert result < IR_NUM_SCALAR_REGS.i32()`
- `shader_recompiler/ir/reg.elisa:63` `ShaderIrVectorReg_Add`: `assert result < IR_NUM_VECTOR_REGS.i32()`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:120` `shader_ir_emitter_accepts_refined_register_indices`: `assert ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 12`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:123` `shader_ir_emitter_accepts_refined_register_indices`: `assert ShaderIrValue_ScalarReg(block.insts[1].args[0]) == 12`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:127` `shader_ir_emitter_accepts_refined_register_indices`: `assert ShaderIrValue_VectorReg(block.insts[2].args[0]) == 240`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:131` `shader_ir_emitter_accepts_refined_register_indices`: `assert ShaderIrValue_VectorReg(block.insts[4].args[0]) == 240`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:144` `shader_ir_values_roundtrip_refined_register_indices`: `assert ShaderIrValue_TryScalarRegIndex(scalar_value, scalar_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:145` `shader_ir_values_roundtrip_refined_register_indices`: `assert ShaderIrValue_ScalarReg(scalar_value) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:147` `shader_ir_values_roundtrip_refined_register_indices`: `assert not ShaderIrValue_TryScalarRegIndex(vector_value, scalar_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:149` `shader_ir_values_roundtrip_refined_register_indices`: `assert ShaderIrValue_TryVectorRegIndex(vector_value, vector_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:150` `shader_ir_values_roundtrip_refined_register_indices`: `assert ShaderIrValue_VectorReg(vector_value) == 255`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:152` `shader_ir_values_roundtrip_refined_register_indices`: `assert not ShaderIrValue_TryVectorRegIndex(scalar_value, vector_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:164` `shader_ir_raw_register_values_remain_compatibility_only`: `assert ShaderIrValue_ScalarReg(scalar_raw) == 104`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:165` `shader_ir_raw_register_values_remain_compatibility_only`: `assert ShaderIrValue_VectorReg(vector_raw) == 256`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:167` `shader_ir_raw_register_values_remain_compatibility_only`: `assert not ShaderIrValue_TryScalarRegIndex(scalar_raw, scalar_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:168` `shader_ir_raw_register_values_remain_compatibility_only`: `assert not ShaderIrValue_TryVectorRegIndex(vector_raw, vector_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:169` `shader_ir_raw_register_values_remain_compatibility_only`: `assert not ShaderIrValue_TryScalarRegIndex(vector_raw, scalar_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:170` `shader_ir_raw_register_values_remain_compatibility_only`: `assert not ShaderIrValue_TryVectorRegIndex(scalar_raw, vector_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:189` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 104`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:190` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert not ShaderIrValue_TryScalarRegIndex(block.insts[0].args[0], scalar_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:193` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert ShaderIrValue_VectorReg(block.insts[1].args[0]) == 256`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:194` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert not ShaderIrValue_TryVectorRegIndex(block.insts[1].args[0], vector_roundtrip)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:215` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:219` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert ShaderIrValue_ScalarReg(block.insts[3].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:221` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert ShaderIrValue_VectorReg(block.insts[4].args[0]) == 255`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:225` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert ShaderIrValue_VectorReg(block.insts[7].args[0]) == 255`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:237` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrScalarRegIndex_TryFromU32(103, max_sgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:238` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrVectorRegIndex_TryFromU32(255, max_vgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:242` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert not ShaderIrScalarRegIndex_TryFromU32(104, invalid_sgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:243` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert not ShaderIrVectorRegIndex_TryFromU32(256, invalid_vgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:254` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:259` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_ScalarReg(block.insts[1].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:263` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_VectorReg(block.insts[2].args[0]) == 255`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:268` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_VectorReg(block.insts[3].args[0]) == 255`
- `elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa:26` `shader_frontend_instruction_refined_register_index_contracts`: `assert GcnReadScalarRegister(sidx.value, sgprs) == 0x1234`
- `elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa:30` `shader_frontend_instruction_refined_register_index_contracts`: `assert GcnReadVectorRegister(vidx.value, vgprs) == 0x5678`
- `elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa:36` `shader_frontend_instruction_refined_register_index_contracts`: `assert GcnVectorRegisterPairIndex_Try(255).has_value == false`
- `elisa_tests/shader_recompiler_frontend_instruction_index_tests.elisa:39` `shader_frontend_instruction_refined_register_index_contracts`: `assert InstOperand_TryScalarRegisterIndex(vector_operand).has_value == false`

