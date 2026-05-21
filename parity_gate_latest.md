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
- fallback: 149
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
- imports total: 1151
- native HLE count: 691
- PRX export count: 311
- AeroLib fallback count: 149
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
- [1] nid=4tPhsP6FpDI lib=f module=f source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=KK3Bdg1RWK0 lib=f module=f source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=YuH2FA7azqQ lib=f module=f source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=s9e3+YpRnzw lib=f module=f source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=eR2bZFAAU0Q lib=j module=j source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=g8cM39EUZ6o lib=j module=j source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=656LMQSrg6U lib=k module=k source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Vo5V8KAwCmk lib=k module=k source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=fZo48un7LK4 lib=k module=k source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=rPo6tV8D9bM lib=k module=k source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=1xxcMiGu2fo lib=l module=l source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=yH17Q6NWtVg lib=l module=l source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=KHvkPQJDMLk lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=OOFxrMY+mfI lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=bGYkY6q3bIw lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=fZJQzFK4Gv4 lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=s28dalBwp2g lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=tWoe9IlGAhs lib=n module=n source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=FraP7debcdg lib=o module=o source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=h1dR-t5ISgg lib=o module=o source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=jqb7HntFQFc lib=o module=o source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=ocHtyBwHfys lib=o module=o source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=+B-xlbiWDJ4 lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=+FYcYefsVX0 lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=1+DgKL0haWQ lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=7rogx92EEyc lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Avv7OApgCJk lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=BkjBP+YC19w lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=FXP359ygujs lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=JQKWIsS9joE lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=MO24vDhmS4E lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=N1EBMeGhf7E lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=ObkDGDBsVtw lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=OqQKX0h5COw lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=PEjv7CVDRYs lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=SSCaczu2aMQ lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=SsRbbCiWoGw lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=W-2WOXEHGck lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=WaSFJoRWXaI lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=cKYtVmeSTcw lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=eCRMCSk96NU lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=exAxkyVLt0s lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=f4Onl7efPEY lib=r module=r source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
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
- [1] 4tPhsP6FpDI f f (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] KK3Bdg1RWK0 f f (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] YuH2FA7azqQ f f (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] s9e3+YpRnzw f f (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] eR2bZFAAU0Q j j (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] g8cM39EUZ6o j j (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 656LMQSrg6U k k (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Vo5V8KAwCmk k k (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] fZo48un7LK4 k k (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] rPo6tV8D9bM k k (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 1xxcMiGu2fo l l (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] yH17Q6NWtVg l l (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] KHvkPQJDMLk n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] OOFxrMY+mfI n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] bGYkY6q3bIw n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] fZJQzFK4Gv4 n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] s28dalBwp2g n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] tWoe9IlGAhs n n (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
### graphics_fallbacks
- none
### audio_input_service_fallbacks
- none
### loader_guest_exec_blockers
- none

## Fallback Delta
- fallback count before: 154
- fallback count after: 149
- fallback symbols newly resolved: 5

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
