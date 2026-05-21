# Emulator C++ Parity Gate

Passed: 15
Failed: 0

Summary score: 19/20
Per-subsystem counts:
- External-C-ABI: 9
- Missing: 1
- Native-Elisa: 247
- Stub-Parity: 30
- Temporary-Cpp-Bridge: 2
Parity gate counts (native/bridge/fallback/unresolved):
- native: 247
- bridge: 2
- fallback: 21
- unresolved: 0
Ledger risk counts (missing/unverified):
- missing: 1
- unverified: 30
CUSA07399 stage:
- execution stage raw: 4
- load: PASS
- link: PASS
- handoff: PASS
- execute stage: guarded-entry
- first boundary: PENDING
- synthetic renderer smoke: PASS
- real CUSA first frame: PENDING
- first frame ladder: none
- host note: arm64 macOS probe-only
- guest exec host arch: arm64
- guest exec host mode: macOS
- guest exec supported native execution: 0
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
- native HLE count: 819
- PRX export count: 310
- AeroLib fallback count: 21
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
- current execution stage: 4 (guarded-entry)
- last HLE call: module=libc symbol=RpQJJVKTiFM
- current video/audio/input stage: graphics=0 audio-input-service=0
Top 50 fallback symbols:
- [1] nid=-hJRce8wn1U lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Cxwy7wHq4J0 lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=OcAgPxcq5Vk lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=PR5k1penBLM lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=RujUxbr3haM lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=cK6bYHf-Q5E lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=i-XwZjw0OOY lib=J module=K source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=WlyEA-sLDf0 lib=C module=D source=/app0/sce_module/libSceFios2.prx subsystem=kernel status=AeroLibFallback
- [1] nid=ZT4ODD2Ts9o lib=D module=E source=/app0/sce_module/libSceFios2.prx subsystem=kernel status=AeroLibFallback
- [1] nid=ZT4ODD2Ts9o lib=c module=a source=/app0/sce_module/libSceNpToolkit.prx subsystem=kernel status=AeroLibFallback
- [1] nid=__start__Ztext lib= module= source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=__stop__Ztext lib= module= source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=-YTW+qXc3CQ lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=Fjc4-n1+y2g lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=VADc3MNQ3cM lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=Wl2o5hOVZdw lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=crb5j7mkk1c lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=djxxOmW6-aw lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=hHlZQUnlxSM lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=kbw4UHHSYy0 lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
- [1] nid=mo0bFmWppIw lib=A module=B source=/app0/sce_module/libc.prx subsystem=kernel status=AeroLibFallback
Top blockers:
- none
Failing scenario ids:
- none

## Agent Work Queues
### kernel_fallbacks
- [1] -hJRce8wn1U J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Cxwy7wHq4J0 J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] OcAgPxcq5Vk J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] PR5k1penBLM J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] RujUxbr3haM J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] cK6bYHf-Q5E J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] i-XwZjw0OOY J K (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] WlyEA-sLDf0 C D (/app0/sce_module/libSceFios2.prx) subsystem=kernel status=AeroLibFallback
- [1] ZT4ODD2Ts9o D E (/app0/sce_module/libSceFios2.prx) subsystem=kernel status=AeroLibFallback
- [1] ZT4ODD2Ts9o c a (/app0/sce_module/libSceNpToolkit.prx) subsystem=kernel status=AeroLibFallback
- [1] __start__Ztext   (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] __stop__Ztext   (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] -YTW+qXc3CQ A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] Fjc4-n1+y2g A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] VADc3MNQ3cM A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] Wl2o5hOVZdw A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] crb5j7mkk1c A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] djxxOmW6-aw A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] hHlZQUnlxSM A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] kbw4UHHSYy0 A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
- [1] mo0bFmWppIw A B (/app0/sce_module/libc.prx) subsystem=kernel status=AeroLibFallback
### graphics_fallbacks
- none
### audio_input_service_fallbacks
- none
### loader_guest_exec_blockers
- none

## Fallback Delta
- fallback count before: 23
- fallback count after: 21
- fallback symbols newly resolved: 2

## Steps
- PASS: project.json syntax
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: elisacore test core-libraries-audio-parity-tests
- PASS: elisacore test core-libraries-np-parity-tests
- PASS: native guest_exec_runtime warnings
- PASS: native kernel_threads_runtime warnings
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-runtime-services-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
- PASS: elisacore test emulator-renderer-first-frame-smoke
