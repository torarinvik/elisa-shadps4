# Lowered refined register bounds audit

Scope: focused shader frontend/IR register-index tests and generated lowered output.

Result: refined SGPR/VGPR read/write and IR get/set register paths do not contain lowered `104`/`256` register-file bounds checks in the audited wrapper bodies.

Remaining checks are constructor/proof-frontier checks (`TryFromU32`, operand decoding, pair validation), type-tag assertions on `ShaderIrValue_*Reg*` accessors, and unrelated IR structure checks such as arg/block/use-count validation.

## Unexpected refined-path SGPR/VGPR bound checks

None.

## Check-like sites inside refined register path wrappers

- `shader_recompiler_ir_core_pure_tests.lowered.elisa:9063` `ShaderIrEmitter_SetScalarRegIndex`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:9088` `ShaderIrEmitter_SetVectorRegIndex`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:9052` `ShaderIrEmitter_SetScalarReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:9077` `ShaderIrEmitter_SetVectorReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:2887` `ShaderIrEmitter_SetScalarRegIndex`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:2912` `ShaderIrEmitter_SetVectorRegIndex`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:2876` `ShaderIrEmitter_SetScalarReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:2901` `ShaderIrEmitter_SetVectorReg`: `if (value.type_bits == IR_TYPE_F32):`
- `shader_recompiler/ir/ir_emitter.elisa:265` `ShaderIrEmitter_SetScalarRegIndex`: `if value.type_bits == IR_TYPE_F32:`
- `shader_recompiler/ir/ir_emitter.elisa:292` `ShaderIrEmitter_SetVectorRegIndex`: `if value.type_bits == IR_TYPE_F32:`
- `shader_recompiler/ir/ir_emitter.elisa:254` `ShaderIrEmitter_SetScalarReg`: `if value.type_bits == IR_TYPE_F32:`
- `shader_recompiler/ir/ir_emitter.elisa:281` `ShaderIrEmitter_SetVectorReg`: `if value.type_bits == IR_TYPE_F32:`

## Expected remaining register validation sites

- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7760` `ShaderIrScalarRegIndex_TryFromU32`: `if (reg < IR_NUM_SCALAR_REGS):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7766` `ShaderIrVectorRegIndex_TryFromU32`: `if (reg < IR_NUM_VECTOR_REGS):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8068` `ShaderIrValue_ScalarReg`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8083` `ShaderIrValue_VectorReg`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8077` `ShaderIrValue_ScalarRegIndex`: `can Abort.Panic:`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8078` `ShaderIrValue_ScalarRegIndex`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8079` `ShaderIrValue_ScalarRegIndex`: `assert((value.scalar_reg <= 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8092` `ShaderIrValue_VectorRegIndex`: `can Abort.Panic:`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8093` `ShaderIrValue_VectorRegIndex`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8094` `ShaderIrValue_VectorRegIndex`: `assert((value.vector_reg <= 255))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8072` `ShaderIrValue_TryScalarRegIndex`: `if (value.type_bits != IR_TYPE_SCALARREG):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:8087` `ShaderIrValue_TryVectorRegIndex`: `if (value.type_bits != IR_TYPE_VECTORREG):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1584` `ShaderIrScalarRegIndex_TryFromU32`: `if (reg < IR_NUM_SCALAR_REGS):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1590` `ShaderIrVectorRegIndex_TryFromU32`: `if (reg < IR_NUM_VECTOR_REGS):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1892` `ShaderIrValue_ScalarReg`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1907` `ShaderIrValue_VectorReg`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1901` `ShaderIrValue_ScalarRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1902` `ShaderIrValue_ScalarRegIndex`: `assert((value.type_bits == IR_TYPE_SCALARREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1903` `ShaderIrValue_ScalarRegIndex`: `assert((value.scalar_reg <= 103))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1916` `ShaderIrValue_VectorRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1917` `ShaderIrValue_VectorRegIndex`: `assert((value.type_bits == IR_TYPE_VECTORREG))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1918` `ShaderIrValue_VectorRegIndex`: `assert((value.vector_reg <= 255))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1896` `ShaderIrValue_TryScalarRegIndex`: `if (value.type_bits != IR_TYPE_SCALARREG):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1911` `ShaderIrValue_TryVectorRegIndex`: `if (value.type_bits != IR_TYPE_VECTORREG):`
- `shader_recompiler/ir/reg.elisa:30` `ShaderIrScalarRegIndex_TryFromU32`: `if reg < IR_NUM_SCALAR_REGS:`
- `shader_recompiler/ir/reg.elisa:36` `ShaderIrVectorRegIndex_TryFromU32`: `if reg < IR_NUM_VECTOR_REGS:`
- `shader_recompiler/ir/value.elisa:199` `ShaderIrValue_ScalarReg`: `assert value.type_bits == IR_TYPE_SCALARREG`
- `shader_recompiler/ir/value.elisa:217` `ShaderIrValue_VectorReg`: `assert value.type_bits == IR_TYPE_VECTORREG`
- `shader_recompiler/ir/value.elisa:210` `ShaderIrValue_ScalarRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/value.elisa:211` `ShaderIrValue_ScalarRegIndex`: `assert value.type_bits == IR_TYPE_SCALARREG`
- `shader_recompiler/ir/value.elisa:212` `ShaderIrValue_ScalarRegIndex`: `assert value.scalar_reg <= 103`
- `shader_recompiler/ir/value.elisa:228` `ShaderIrValue_VectorRegIndex`: `can Abort.Panic:`
- `shader_recompiler/ir/value.elisa:229` `ShaderIrValue_VectorRegIndex`: `assert value.type_bits == IR_TYPE_VECTORREG`
- `shader_recompiler/ir/value.elisa:230` `ShaderIrValue_VectorRegIndex`: `assert value.vector_reg <= 255`
- `shader_recompiler/ir/value.elisa:204` `ShaderIrValue_TryScalarRegIndex`: `if value.type_bits != IR_TYPE_SCALARREG:`
- `shader_recompiler/ir/value.elisa:222` `ShaderIrValue_TryVectorRegIndex`: `if value.type_bits != IR_TYPE_VECTORREG:`
- `shader_recompiler/frontend/instruction.elisa:383` `InstOperand_TryScalarRegisterIndex`: `if op.field == OPERAND_FIELD_SCALAR_GPR and op.has_refined_sgpr and op.refined_sgpr <= OPERAND_FIELD_RANGE_SCALAR_GPR_MAX:`
- `shader_recompiler/frontend/instruction.elisa:391` `InstOperand_TryVectorRegisterIndex`: `if op.field == OPERAND_FIELD_VECTOR_GPR and op.has_refined_vgpr:`
- `shader_recompiler/frontend/instruction.elisa:335` `GcnScalarRegisterIndex_Try`: `if value <= 127:`
- `shader_recompiler/frontend/instruction.elisa:343` `GcnVectorRegisterIndex_Try`: `if value <= 255:`
- `shader_recompiler/frontend/instruction.elisa:359` `GcnVectorRegisterPairIndex_Try`: `if value <= 254:`

## All focused register-adjacent check-like sites

- `shader_recompiler_ir_core_pure_tests.lowered.elisa:5454` `elisa_trace_dump`: `if (wn > 256):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7760` `ShaderIrScalarRegIndex_TryFromU32`: `if (reg < IR_NUM_SCALAR_REGS):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7766` `ShaderIrVectorRegIndex_TryFromU32`: `if (reg < IR_NUM_VECTOR_REGS):`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7781` `ShaderIrScalarReg_Add`: `assert((result < IR_NUM_SCALAR_REGS.i32()))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7785` `ShaderIrScalarReg_Sub`: `return ShaderIrScalarReg_Add(reg, (-num)) can Abort.Panic`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7791` `ShaderIrVectorReg_Add`: `assert((result < IR_NUM_VECTOR_REGS.i32()))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:7795` `ShaderIrVectorReg_Sub`: `return ShaderIrVectorReg_Add(reg, (-num)) can Abort.Panic`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14400` `shader_ir_emitter_accepts_refined_register_indices`: `assert((ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 12))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14402` `shader_ir_emitter_accepts_refined_register_indices`: `assert((ShaderIrValue_ScalarReg(block.insts[1].args[0]) == 12))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14405` `shader_ir_emitter_accepts_refined_register_indices`: `assert((ShaderIrValue_VectorReg(block.insts[2].args[0]) == 240))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14408` `shader_ir_emitter_accepts_refined_register_indices`: `assert((ShaderIrValue_VectorReg(block.insts[4].args[0]) == 240))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14420` `shader_ir_values_roundtrip_refined_register_indices`: `assert(ShaderIrValue_TryScalarRegIndex(scalar_value, scalar_roundtrip))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14421` `shader_ir_values_roundtrip_refined_register_indices`: `assert((ShaderIrValue_ScalarReg(scalar_value) == 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14423` `shader_ir_values_roundtrip_refined_register_indices`: `assert((not ShaderIrValue_TryScalarRegIndex(vector_value, scalar_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14424` `shader_ir_values_roundtrip_refined_register_indices`: `assert(ShaderIrValue_TryVectorRegIndex(vector_value, vector_roundtrip))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14425` `shader_ir_values_roundtrip_refined_register_indices`: `assert((ShaderIrValue_VectorReg(vector_value) == 255))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14427` `shader_ir_values_roundtrip_refined_register_indices`: `assert((not ShaderIrValue_TryVectorRegIndex(scalar_value, vector_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14439` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((ShaderIrValue_ScalarReg(scalar_raw) == 104))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14440` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((ShaderIrValue_VectorReg(vector_raw) == 256))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14441` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((not ShaderIrValue_TryScalarRegIndex(scalar_raw, scalar_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14442` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((not ShaderIrValue_TryVectorRegIndex(vector_raw, vector_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14443` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((not ShaderIrValue_TryScalarRegIndex(vector_raw, scalar_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14444` `shader_ir_raw_register_values_remain_compatibility_only`: `assert((not ShaderIrValue_TryVectorRegIndex(scalar_raw, vector_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14461` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert((ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 104))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14462` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert((not ShaderIrValue_TryScalarRegIndex(block.insts[0].args[0], scalar_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14464` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert((ShaderIrValue_VectorReg(block.insts[1].args[0]) == 256))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14465` `shader_ir_raw_emitter_adapters_are_compatibility_only`: `assert((not ShaderIrValue_TryVectorRegIndex(block.insts[1].args[0], vector_roundtrip)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14484` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert((ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14488` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert((ShaderIrValue_ScalarReg(block.insts[3].args[0]) == 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14490` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert((ShaderIrValue_VectorReg(block.insts[4].args[0]) == 255))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14494` `shader_ir_emitter_refined_f32_register_adapters_are_named_and_bounded`: `assert((ShaderIrValue_VectorReg(block.insts[7].args[0]) == 255))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14508` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert(ShaderIrScalarRegIndex_TryFromU32(103, narrowed_sgpr))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14510` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert(ShaderIrVectorRegIndex_TryFromU32(255, narrowed_vgpr))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14514` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((not ShaderIrScalarRegIndex_TryFromU32(104, invalid_sgpr)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14515` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((not ShaderIrVectorRegIndex_TryFromU32(256, invalid_vgpr)))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14524` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14528` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((ShaderIrValue_ScalarReg(block.insts[1].args[0]) == 103))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14531` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((ShaderIrValue_VectorReg(block.insts[2].args[0]) == 255))`
- `shader_recompiler_ir_core_pure_tests.lowered.elisa:14535` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert((ShaderIrValue_VectorReg(block.insts[3].args[0]) == 255))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1584` `ShaderIrScalarRegIndex_TryFromU32`: `if (reg < IR_NUM_SCALAR_REGS):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1590` `ShaderIrVectorRegIndex_TryFromU32`: `if (reg < IR_NUM_VECTOR_REGS):`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1605` `ShaderIrScalarReg_Add`: `assert((result < IR_NUM_SCALAR_REGS.i32()))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1609` `ShaderIrScalarReg_Sub`: `return ShaderIrScalarReg_Add(reg, (-num)) can Abort.Panic`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1615` `ShaderIrVectorReg_Add`: `assert((result < IR_NUM_VECTOR_REGS.i32()))`
- `shader_recompiler/ir/ir_emitter.lowered.elisa:1619` `ShaderIrVectorReg_Sub`: `return ShaderIrVectorReg_Add(reg, (-num)) can Abort.Panic`
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
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:239` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrScalarRegIndex_TryFromU32(103, narrowed_sgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:241` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrVectorRegIndex_TryFromU32(255, narrowed_vgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:246` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert not ShaderIrScalarRegIndex_TryFromU32(104, invalid_sgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:247` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert not ShaderIrVectorRegIndex_TryFromU32(256, invalid_vgpr)`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:258` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_ScalarReg(block.insts[0].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:263` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_ScalarReg(block.insts[1].args[0]) == 103`
- `elisa_tests/shader_recompiler_ir_core_pure_tests.elisa:267` `shader_ir_refined_register_index_boundary_elides_recheck_regression`: `assert ShaderIrValue_VectorReg(block.insts[2].args[0]) == 255`
- ... 6 more

