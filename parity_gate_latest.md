# Emulator C++ Parity Gate

Passed: 23
Failed: 4

Ledger statuses:
- External-C-ABI: 11
- Implemented: 237
- Stub-Parity: 40
Matrix snapshots:
- core_libraries_requested_scope_parity_matrix.json: MATCHED=93, MISSING_IN_ELISA=1
- system_parity_matrix.json: MATCHED=930
- imgui_big_picture_parity_matrix.json: MATCHED=85
- shader_recompiler_ir_passes_parity_matrix.json: MATCHED=25
- shader_recompiler_ir_core_parity_matrix.json: MATCHED=285
- shader_recompiler_backend_spirv_parity_matrix.json: MATCHED=995
CUSA07399: where are we?
- load: FAIL
- link: FAIL
- handoff: PASS
- guarded entry: PENDING
- first boundary: PENDING
- first frame: PENDING
CUSA07399 artifact metrics:
- module count: 0
- total imports: 0
- native HLE imports: 0
- PRX export imports: 0
- AeroLib fallback imports: 0
- unresolved imports: 0
- loaded modules: 0
- relocation count: 0
Post-boot runtime readiness:
- pad: native-Elisa (pad runtime now consumes shared Elisa controller state with touch/motion/lightbar/vibration paths; evidence: core/libraries/pad/pad.elisa, input/controller.elisa, input/input_handler.elisa)
- audioout: native-Elisa (runtime state and backend abstraction are present; host audio device open is not required; evidence: core/libraries/audio/audioout.elisa, core/libraries/audio/audioout_backend.elisa)
- savedata: C++-bridge (mount/param behavior is exposed through the parity bridge; evidence: core/libraries/save_data/savedata.elisa, core/libraries/save_data/savedata_ffi.elisa, core/libraries/save_data/savedata_elisa_bridge.cpp)
- userservice/systemservice: C-FFI (service metadata and FFI shims are available for post-boot probing; evidence: core/libraries/system/userservice.elisa, core/libraries/system/systemservice.elisa, core/libraries/system/userservice_ffi_bridge.elisa, core/libraries/system/systemservice_ffi_bridge.elisa)
- netctl/network: stub-parity (non-invasive netctl defaults plus socket conversion FFI; no live network dependency; evidence: core/libraries/network/netctl.elisa, core/libraries/network/net.elisa, core/libraries/network/net_ffi.elisa)
- NP: stub-parity (NP runtime and module stubs are visible for real-game import readiness; evidence: core/libraries/np/np.elisa, core/libraries/np/np_runtime.elisa)
- avplayer: C-FFI (FFmpeg-backed media probe surface exists; media decode remains explicit subsystem work; evidence: core/libraries/avplayer/avplayer_impl.elisa, core/libraries/avplayer/avplayer_ffmpeg_ffi.elisa)

## Steps
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: generate core_libraries_requested_scope_parity_matrix
- FAIL: check core_libraries_requested_scope_parity_matrix
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
- FAIL: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-guest-exec-runtime-tests
- FAIL: elisacore test emulator-core-boot-smoke
- FAIL: elisacore test emulator-real-game-boot-smoke
