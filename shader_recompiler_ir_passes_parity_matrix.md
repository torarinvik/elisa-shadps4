# Shader Recompiler IR Passes Parity Matrix

## Summary

- Total symbols: `25`
- MATCHED: `25`
- MISMATCH_BEHAVIOR: `0`
- MISSING_IN_ELISA: `0`
- UNTESTED_BEHAVIOR: `0`

## Entries

| Symbol | Status | C++ | Elisa | Tests |
|---|---|---|---|---|
| `CalculateSharedMemoryTypes` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `CalculateSpecialSharedAtomicTypes` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `CollectShaderInfoPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `ConstantPropagation` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `ConstantPropagationPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `DeadCodeEliminationPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `DomainShaderTransform` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `FlattenExtendedUserdataPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `HullShaderTransform` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `IdentityRemovalPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `InjectClipDistanceAttributes` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `Lower` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `LowerBufferFormatToRaw` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `LowerFp64ToFp32` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `ReadLaneEliminationPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `RegisterWalkerCode` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | shader_recompiler_ir_passes_pure_tests.elisa |
| `ResourceTrackingPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `RingAccessElimination` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `SharedMemoryBarrierPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `SharedMemorySimplifyPass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `SharedMemoryToStoragePass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `SrtWalkerSignalHandler` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | shader_recompiler_ir_passes_pure_tests.elisa |
| `SsaRewritePass` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `TessellationPreprocess` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
| `Visit` | `MATCHED` | `STATEFUL` | `STATEFUL` | shader_recompiler_ir_passes_pure_tests.elisa |
