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
- fallback: 293
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
- imports total: 1152
- native HLE count: 547
- PRX export count: 312
- AeroLib fallback count: 293
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
- [1] nid=12wOHk8ywb0 lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=188x57JYp0g lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=4czppHBiriw lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=PrdHuuDekhY lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=R1Jvn8bSCW8 lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=Zxa0VhQVTsk lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=eoht7mQOCmo lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=f7uOxY9mM1U lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=geDaqgH9lTg lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=qBDmpCyGssE lib=BH module=d source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=FCygF4Ec4so lib=E module=F source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=OWCJUmrWH1g lib=E module=F source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=tb3cZTCl8Ps lib=E module=F source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=+AFvOEXrKJk lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=+xuDhxlWRPg lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=0BzLGljcwBo lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=1qXLHIpROPE lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=26PM5Mzl8zc lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=29oKvKXzEZo lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=4v+otIIdjqg lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=5q95ravnueg lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=5uFKckiJYRM lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=5udAm+6boVg lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
- [1] nid=6YRHhh5mHCs lib=G module=H source=/app0/eboot.bin subsystem=kernel status=AeroLibFallback
Top blockers:
- none
Failing scenario ids:
- none

## Agent Work Queues
### kernel_fallbacks
- [1] -Q1-u1a7p0g BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] -RJWNMK3fC8 BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 4AAcTU9R3XM BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] M1Gma1ocrGE BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] MPe0EeBGM-E BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Nn7zKwnA5q0 BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Uco1I0dlDi8 BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] gUPGiOQ1tmQ BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] rvBSfTimejE BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] ts6GlZOKRrE BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] uWIYLFkkwqk BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] v6EZ-YWRdMs BB c (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 2xxUtuC-RzE BD v (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] tIYf0W5VTi8 BD v (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 54phPH2LZls BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] 9TrhuGzberQ BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Ao2YNSA7-Qo BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] CrLqDwWLoXM BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] Oo0S5PH7FIQ BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] YeJl6yDlhW0 BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] ajVj3QG2um4 BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] b7kJI+nx2hg BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] cQ6DGsQEjV4 BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] nXpje5yNpaE BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
- [1] oV9GAdJ23Gw BG x (/app0/eboot.bin) subsystem=kernel status=AeroLibFallback
### graphics_fallbacks
- none
### audio_input_service_fallbacks
- none
### loader_guest_exec_blockers
- none

## Fallback Delta
- fallback count before: 293
- fallback count after: 293
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
- PASS: native kernel_threads_runtime warnings
- PASS: elisacore test emulator-core-boot-smoke
- PASS: elisacore test real-self-loader-tests
- PASS: elisacore test emulator-real-game-boot-smoke
- PASS: elisacore test emulator-runtime-services-smoke
- PASS: elisacore test emulator-guest-exec-runtime-tests
- PASS: elisacore test emulator-renderer-first-frame-smoke
