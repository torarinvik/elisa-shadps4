# Emulator C++ Parity Gate

Passed: 10
Failed: 1

Summary score: 14/16
Per-subsystem counts:
- External-C-ABI: 4
- Native-Elisa: 255
- Stub-Parity: 30
Parity gate counts (native/bridge/fallback/unresolved):
- native: 255
- bridge: 0
- fallback: 0
- unresolved: 0
Ledger risk counts (missing/unverified):
- missing: 0
- unverified: 30
CUSA07399 stage:
- execution stage raw: 6
- load: FAIL
- link: FAIL
- handoff: FAIL
- execute stage: crash-captured
- first boundary: PASS
- synthetic renderer smoke: FAIL
- real CUSA first frame: PENDING
- first frame ladder: none
- host note: arm64 macOS probe-only
- guest exec host arch: x86_64
- guest exec host mode: macOS
- guest exec supported native execution: 1
- guest exec x64 subprocess available: 1
- guest exec x64 subprocess smoke: FAIL
- CUSA07399 x64 real execution lane: missing-output
- guest exec probe only: 0
- guest exec started: 1
- guest exec entry reached: 1
- guest exec first boundary reached: 0
- guest exec boundary reason: 4 (host-crash)
- guest exec boundary reason name: host-crash
- guest exec last pc: 0x800002030
- guest exec last sp: 0x7efdd3fd8
- guest exec last bp: 0x70020513bb90
- guest exec last rdi: 0x70020513b6e8
- guest exec expected EntryParams: 0x70020513b6e8
- guest exec expected stack words: 0x0, 0x0
- guest exec entry stack words: 0x0, 0x0
- guest exec fault words: 0x80050c660, 0x80050c710
- guest exec signal stack words: 0x0, 0x0
- guest exec last signal: 4
- guest exec last module: 
- guest exec last symbol: 
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
- imports total: 1154
- native HLE count: 850
- PRX export count: 304
- AeroLib fallback count: 0
- unresolved count: 0
- malformed count: 0
- audio SDL available: 0
- audio OpenAL available: 0
Real CUSA runtime service signals:
- real CUSA user service initialized: 0
- user service initial user: 0
- user service login user count: 0
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
- current execution stage: 6 (crash-captured)
- last HLE call: module= symbol=
- current video/audio/input stage: graphics=0 audio-input-service=0
- current save/dialog/misc fallback stage: save-dialog-misc=0
Save/Dialog/Misc parity test signals:
- save data parity tests: FAIL
- save data dialog parity tests: FAIL
- web browser dialog parity tests: FAIL
- signin dialog parity tests: FAIL
- playgo parity tests: FAIL
- ime dialog parity tests: FAIL
Top 50 fallback symbols:
- none
Top blockers:
- Gate step failed: CUSA07399 x64 real execution lane
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
- PASS: emulator ABI smoke
- PASS: strict native ABI contracts
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: elisacore test core-libraries-audio-parity-tests
- PASS: elisacore test core-libraries-np-parity-tests
- PASS: guest exec ABI smoke x64
- FAIL: CUSA07399 x64 real execution lane
