# Emulator C++ Parity Gate

Passed: 27
Failed: 0

Ledger statuses:
- External-C-ABI: 11
- Implemented: 237
- Stub-Parity: 40
Matrix snapshots:
- core_libraries_requested_scope_parity_matrix.json: MATCHED=94
- system_parity_matrix.json: MATCHED=930
- imgui_big_picture_parity_matrix.json: MATCHED=85
- shader_recompiler_ir_passes_parity_matrix.json: MATCHED=25
- shader_recompiler_ir_core_parity_matrix.json: MATCHED=285
- shader_recompiler_backend_spirv_parity_matrix.json: MATCHED=995
CUSA07399 staged gate:
- load/link/handoff: PASS
- execute-fixture: PASS
- execute-game: UNSUPPORTED_HOST (arm64)
- frame: not yet promoted into this gate

## Steps
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: generate core_libraries_requested_scope_parity_matrix
- PASS: check core_libraries_requested_scope_parity_matrix
- PASS: generate system_parity_matrix
- PASS: check system_parity_matrix
- PASS: generate imgui_big_picture_parity_matrix
- PASS: check imgui_big_picture_parity_matrix
- PASS: generate shader_recompiler_ir_passes_parity_matrix
- PASS: check shader_recompiler_ir_passes_parity_matrix
- PASS: generate shader_recompiler_ir_core_parity_matrix
- PASS: check shader_recompiler_ir_core_parity_matrix
- PASS: generate shader_recompiler_backend_spirv_parity_matrix
- PASS: check shader_recompiler_backend_spirv_parity_matrix
- PASS: project.json syntax
- PASS: native guest_exec_runtime warnings
- PASS: native kernel_threads_runtime warnings
- PASS: native kernel_threads_runtime_test_support warnings
- PASS: elisacore test core-platform-tests
- PASS: elisacore test video-core-multi-level-page-table-tests
- PASS: elisacore test video-core-renderer-vulkan-diagnostics-tests
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-guest-exec-runtime-tests
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test emulator-real-game-boot-smoke
