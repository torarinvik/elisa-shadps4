# Emulator C++ Parity Gate

Passed: 11
Failed: 0

Summary score: 13/16
Per-subsystem counts:
- External-C-ABI: 9
- Missing: 1
- Native-Elisa: 237
- Stub-Parity: 40
- Temporary-Cpp-Bridge: 2
Parity gate counts (native/bridge/fallback/unresolved):
- native: 237
- bridge: 2
- fallback: 0
- unresolved: 0
Ledger risk counts (missing/unverified):
- missing: 1
- unverified: 40
CUSA07399 stage:
- execution stage raw: 0
- load: PASS
- link: PASS
- handoff: PASS
- execute stage: none
- first boundary: PENDING
- first frame: PENDING
- first frame gate signals:
  - shader_translate_attempted=0
  - shader_path_bridge=0 shader_path_native=0
  - VIDEO_STAGE_opened=0
  - VIDEO_STAGE_buffers_registered=0
  - VIDEO_STAGE_flip_submitted=0
  - VIDEO_STAGE_flip_completed=0
  - VIDEO_STAGE_first_frame_candidate=0
CUSA07399 artifact metrics:
- CUSA module count: 0
- imports total: 0
- native HLE count: 0
- PRX export count: 0
- AeroLib fallback count: 0
- unresolved count: 0
- malformed count: 0
- current execution stage: 0 (none)
- last HLE call: module= symbol=
- current video/audio/input stage: graphics=0 audio-input-service=0
Top 25 fallback symbols:
- none
Top blockers:
- none
Failing scenario ids:
- none

## Agent Work Queues
### kernel_fallbacks
- none
### graphics_fallbacks
- none
### audio_input_service_fallbacks
- none
### loader_guest_exec_blockers
- none

## Steps
- PASS: project.json syntax
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: native guest_exec_runtime warnings
- PASS: native kernel_threads_runtime warnings
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
