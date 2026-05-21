# Emulator C++ Parity Gate

Passed: 14
Failed: 0

Summary score: 18/19
Per-subsystem counts:
- External-C-ABI: 9
- Missing: 1
- Native-Elisa: 247
- Stub-Parity: 30
- Temporary-Cpp-Bridge: 2
Parity gate counts (native/bridge/fallback/unresolved):
- native: 247
- bridge: 2
- fallback: 437
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
- imports total: 1153
- native HLE count: 404
- PRX export count: 312
- AeroLib fallback count: 437
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
- [1] nid=0aR2aWmQal4 lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=DHmwsa6S8Tc lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=DfSCDRA3EjY lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=LR5cwFMMCVE lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=dsqCVsNM0Zg lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=m-I92Ab50W8 lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=r42bWcQbtZY lib=0 module=P source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=2rsFmlGWleQ lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=A2CQ3kgSopQ lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=OzKvTvg3ZYU lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=S7QTn72PrDw lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=VgYczPGB5ss lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=XDncXQIJUSk lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=eiqMCt9UshI lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=ilwLM4zOmu4 lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=p-o74CnoNzY lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=rbknaUjpqWo lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=uqcPJLWL08M lib=3 module=S source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=28xmRUFao68 lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=E1Wrwd07Lr8 lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=GNcF4oidY0Y lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=TJCAxto9SEU lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=XbkjbobZlCY lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=q7U6tEAQf7c lib=9 module=X source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=-Q1-u1a7p0g lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=-RJWNMK3fC8 lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=4AAcTU9R3XM lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=M1Gma1ocrGE lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=MPe0EeBGM-E lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Nn7zKwnA5q0 lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Uco1I0dlDi8 lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=gUPGiOQ1tmQ lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=rvBSfTimejE lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=ts6GlZOKRrE lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=uWIYLFkkwqk lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=v6EZ-YWRdMs lib=BB module=c source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=2xxUtuC-RzE lib=BD module=v source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=tIYf0W5VTi8 lib=BD module=v source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=54phPH2LZls lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=9TrhuGzberQ lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Ao2YNSA7-Qo lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=CrLqDwWLoXM lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Oo0S5PH7FIQ lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=YeJl6yDlhW0 lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=ajVj3QG2um4 lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=b7kJI+nx2hg lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=cQ6DGsQEjV4 lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=nXpje5yNpaE lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=oV9GAdJ23Gw lib=BG module=x source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=+BzXYkqYeLE lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
Top blockers:
- none
Failing scenario ids:
- none

## Agent Work Queues
### kernel_fallbacks
- [1] 0aR2aWmQal4 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] DHmwsa6S8Tc 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] DfSCDRA3EjY 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] LR5cwFMMCVE 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] dsqCVsNM0Zg 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] m-I92Ab50W8 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] r42bWcQbtZY 0 P (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 2rsFmlGWleQ 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] A2CQ3kgSopQ 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] OzKvTvg3ZYU 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] S7QTn72PrDw 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] VgYczPGB5ss 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] XDncXQIJUSk 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] eiqMCt9UshI 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] ilwLM4zOmu4 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] p-o74CnoNzY 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] rbknaUjpqWo 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] uqcPJLWL08M 3 S (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 28xmRUFao68 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] E1Wrwd07Lr8 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] GNcF4oidY0Y 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] TJCAxto9SEU 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] XbkjbobZlCY 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] q7U6tEAQf7c 9 X (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] -Q1-u1a7p0g BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
### graphics_fallbacks
- none
### audio_input_service_fallbacks
- none
### loader_guest_exec_blockers
- none

## Fallback Delta
- fallback count before: 437
- fallback count after: 437
- fallback symbols newly resolved: 0

## Steps
- PASS: project.json syntax
- PASS: parity ledger
- PASS: parity ABI guard
- PASS: parity workqueue summary
- PASS: bridge syntax
- PASS: elisacore test core-libraries-audio-parity-tests
- PASS: native guest_exec_runtime warnings
- PASS: native kernel_threads_runtime warnings
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-runtime-services-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
- PASS: elisacore test emulator-renderer-first-frame-smoke
