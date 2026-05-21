# Emulator C++ Parity Gate

Passed: 8
Failed: 3

Summary score: 9/16
Per-subsystem counts:
- External-C-ABI: 11
- Native-Elisa: 237
- Stub-Parity: 40
Parity gate counts (native/bridge/fallback/unresolved):
- native: 237
- bridge: 0
- fallback: 0
- unresolved: 0
CUSA07399 stage:
- execution stage raw: 0
- load: FAIL
- link: FAIL
- handoff: PASS
- execute stage: none
- first boundary: PENDING
- first frame: PENDING
CUSA07399 artifact metrics:
- CUSA module count: 0
- imports total: 0
- native HLE count: 0
- PRX export count: 0
- AeroLib fallback count: 0
- unresolved count: 0
- malformed count: 0
- current execution stage: 0 (none)
- current video/audio/input stage: graphics=0 audio-input-service=0
Top 25 fallback symbols:
- none
Top blockers:
- Gate step failed: elisacore test emulator-core-boot-smoke
- Gate step failed: elisacore test real-self-loader-tests
- Gate step failed: elisacore test emulator-real-game-boot-smoke

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
- FAIL: elisacore test emulator-core-boot-smoke
- FAIL: elisacore test real-self-loader-tests
- FAIL: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
