# Emulator C++ Parity Gate

Passed: 22
Failed: 0

Summary score: 27/27
Per-subsystem counts:
- External-C-ABI: 11
- Native-Elisa: 248
- Stub-Parity: 30
Parity gate counts (native/bridge/fallback/unresolved):
- native: 248
- bridge: 0
- fallback: 0
- unresolved: 0
Ledger risk counts (missing/unverified):
- missing: 0
- unverified: 30
CUSA07399 stage:
- execution stage raw: 4
- load: PASS
- link: PASS
- handoff: PASS
- execute stage: guarded-entry
- first boundary: PASS
- synthetic renderer smoke: PASS
- real CUSA first frame: PENDING
- first frame ladder: none
- host note: arm64 macOS probe-only
- guest exec host arch: arm64
- guest exec host mode: macOS
- guest exec supported native execution: 0
- guest exec x64 subprocess available: 1
- guest exec x64 subprocess smoke: PASS
- guest exec probe only: 1
- guest exec started: 1
- guest exec entry reached: 1
- guest exec first boundary reached: 0
- guest exec boundary reason: 7 (unsupported-host)
- guest exec boundary reason name: unsupported-host
- guest exec last pc: 0x0
- guest exec last signal: 0
- guest exec last module: libc
- guest exec last symbol: RpQJJVKTiFM
- first boundary blocker: probe-only host (arm64 macOS)
- first frame gate signals:
  - shader_translate_attempted=0
  - shader_path_bridge=0 shader_path_native=0
  - VIDEO_STAGE_opened=0
  - VIDEO_STAGE_buffers_registered=0
  - VIDEO_STAGE_flip_submitted=0
  - VIDEO_STAGE_flip_completed=0
  - VIDEO_STAGE_first_frame_candidate=0
CUSA07399 artifact metrics:
- CUSA module count: 4
- imports total: 1150
- native HLE count: 840
- PRX export count: 310
- AeroLib fallback count: 0
- unresolved count: 0
- malformed count: 0
- audio SDL available: 0
- audio OpenAL available: 0
Real CUSA runtime service signals:
- real CUSA user service initialized: 1
- user service initial user: 1
- user service login user count: 2
- real CUSA pad initialized: 0
- pad open attempted/opened: 0/0
- pad no controller: 0
- pad read attempted/rc: 0/0
- pad closed: 0
- real CUSA audio output attempted/opened: 0/0
- audio output write attempted/ok: 0/0
- audio output closed: 0
- real CUSA audio input attempted/opened: 0/0
- audio input read attempted/rc: 0/0
- audio input silent state: 0
- audio input closed: 0
- current execution stage: 4 (guarded-entry)
- last HLE call: module=libc symbol=RpQJJVKTiFM
- current video/audio/input stage: graphics=0 audio-input-service=0
- current save/dialog/misc fallback stage: save-dialog-misc=0
Save/Dialog/Misc parity test signals:
- save data parity tests: PASS
- save data dialog parity tests: PASS
- web browser dialog parity tests: PASS
- signin dialog parity tests: PASS
- playgo parity tests: PASS
- ime dialog parity tests: PASS
Top 50 fallback symbols:
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
### save_dialog_misc_fallbacks
- none
### loader_guest_exec_blockers
- none

## Fallback Delta
- fallback count before: 0
- fallback count after: 0
- fallback symbols newly resolved: 0

## Steps
- PASS: project.json syntax
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: elisacore test core-libraries-audio-parity-tests
- PASS: elisacore test core-libraries-np-parity-tests
- PASS: native guest_exec_runtime warnings
- PASS: guest exec x64 subprocess smoke
- PASS: native kernel_threads_runtime warnings
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-runtime-services-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
- PASS: elisacore test emulator-renderer-first-frame-smoke
- PASS: elisacore run elisa_tests/core_libraries_save_data_parity_tests.elisa
- PASS: elisacore run elisa_tests/core_libraries_save_data_dialog_pure_tests.elisa
- PASS: elisacore run elisa_tests/core_libraries_web_browser_dialog_pure_tests.elisa
- PASS: elisacore run elisa_tests/core_libraries_signin_dialog_pure_tests.elisa
- PASS: elisacore run elisa_tests/core_libraries_playgo_pure_tests.elisa
- PASS: elisacore run elisa_tests/core_libraries_ime_dialog_parity_tests.elisa
