# AvPlayer FFI And Parity Notes

This document tracks Elisa parity status for `core/libraries/avplayer` against
the current C++ implementation and documents how the FFmpeg FFI boundary is
validated.

## Why Direct FFI

The Elisa port uses direct `@c_abi(c)` bindings to FFmpeg instead of a C++
bridge layer so that:

- ownership/lifetime and error handling can stay in Elisa runtime code,
- ABI layout checks are explicit and repeatable during porting,
- parity progress can be measured per behavior instead of per bridge wrapper.

## FFmpeg ABI Validation

Run C binding checks from the compiler workspace:

```bash
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
CPPFLAGS="$(pkg-config --cflags libavformat libavcodec libavutil)" \
go run ./src -emit c-bind-check \
  "../elisa-shad-ps4-from-scratch/core/libraries/avplayer/avplayer_ffmpeg_ffi.elisa"
```

Current validated prefix structs:

- `AVFormatContextPublicPrefix`
- `AVStreamPublicPrefix`
- `AVCodecParametersPublicPrefix`
- `AVChannelLayoutPublicPrefix`
- `AVRationalPublic`

## Parity Matrix (Current)

### Public API wiring (`avplayer.elisa`)

- `Implemented`: init/initEx/addSource/addSourceEx/start/stop/pause/resume/close
- `Implemented`: stream count/info, get audio/video data, loop flag, current time
- `Implemented`: `JumpToTime` matches the current C++ no-op behavior
- `Implemented`: `DisableStream` matches the current C++ no-op behavior
- `Implemented`: subtitle/timed-text streams can participate in auto-start and startup validation
- `Implemented`: handle validation and basic error returns
- `Implemented`: timing uses a monotonic playback clock for `CurrentTime`
- `Partial`: state/event ordering and buffering behavior are simplified vs C++

### Source runtime (`avplayer_source.elisa`)

- `Implemented`: FFmpeg open/close/start/stop/pause/resume calls
- `Implemented`: start path seeks streams back to the beginning before decoder setup
- `Implemented`: stream info extraction (type, duration/start, video/audio details)
- `Implemented`: stream enable/disable based on resolved stream type
- `Implemented`: started/stopped lifecycle tracking and queue-backed pull behavior
- `Implemented`: EOF marking and loop retry on FFmpeg read failure
- `Implemented`: lightweight prefetch queue for video/audio timestamps
- `Implemented`: fallback prefetch reads from selected stream index (stream-directed demux timestamp pull)
- `Implemented`: bounded shared demux fill step services both enabled streams before per-stream read paths
- `Implemented`: explicit demux packet queues (video/audio) with prefetch consuming queued packets before decode fallback
- `Implemented`: per-stream EOF tracking (video/audio) with combined source EOF derived from stream completion + queue emptiness
- `Implemented`: centralized loop restart at demux stage (flush/reset queues + codec flush + FFmpeg restart)
- `Implemented`: per-stream prefetch loop retries removed; loop handling now flows through centralized demux-stage restart
- `Implemented`: bounded decode-driven output fill step before get-audio/get-video return paths
- `Implemented`: source-side cached stream metadata (video dimensions, audio channel/sample rate) used by frame getters with stream-info fallback
- `Implemented`: frame getters now clear stream detail payloads before filling (prevents stale metadata leakage across calls)
- `Implemented`: source tracks delivered frame interval estimates; audio frame `size` now uses cadence-based estimate instead of constant zero
- `Implemented`: source caches stream start-time/duration for active audio/video and frame getters now apply cached stream start-time offset to output timestamps
- `Implemented`: start-time offset application now keys on `start_time` presence (not duration presence), improving streams with unknown/zero duration
- `Implemented`: handle `CurrentTime` bookkeeping now updates from post-offset frame timestamps (internal time matches returned timeline)
- `Implemented`: `CurrentTime` now prefers media-clock timestamps (audio/video decoded output) before wall-clock fallback
- `Implemented`: media-clock `CurrentTime` now includes cached stream start-time offsets for timeline-aligned reporting
- `Implemented`: buffering state updates in frame getters are now source-readiness driven (`avsrc_has_frames`) instead of hardcoded booleans
- `Implemented`: source readiness now treats demux packet queues as frame availability signals (not just decoded output queues)
- `Implemented`: source readiness now only counts queues for enabled stream types (avoids stale-queue false positives after stream mode changes)
- `Implemented`: EOF signaling from frame getters is now unified across audio/video paths using source activity (`avsrc_is_active`)
- `Implemented`: source activity (`IsActive`) now includes demux packet queues, preventing false-inactive states while decode work is buffered
- `Implemented`: source activity queue checks are now filtered by enabled stream types (avoids stale-queue false-active states after stream mode changes)
- `Implemented`: disabling a stream now clears that stream’s output/demux queues and per-stream timing/EOF markers immediately
- `Implemented`: stream-disable path now recomputes combined EOF immediately after per-stream clear operations
- `Implemented`: `IsActive` and `CurrentTime` gating now follow the C++ source shape
- `Partial`: no full demux/decode threads, packet queues, or converted frame pipeline
- `Partial`: no guest buffer pool or frame-object lifetime model like C++

### State/controller (`avplayer_state.elisa`)

- `Implemented`: state enum model, transition checks, add-source/warning/error events
- `Implemented`: basic buffering transition helper and EOF/error state hooks
- `Implemented`: revert-state event type is now represented in the Elisa controller state
- `Implemented`: EOF now flows through controller pending-event handling (queued) instead of immediate state mutation
- `Implemented`: EOF transition validation extended for pause/buffering states
- `Implemented`: per-handle FIFO event queue replaces single pending-event slot (preserves event ordering)
- `Implemented`: EOF transition support extended through `STARTING`; buffering updates now ignore terminal/error states
- `Implemented`: queue-full policy now prefers higher-priority events (`ERROR`/`EOF`) over low-priority events
- `Implemented`: controller now processes events in bounded bursts per tick (FIFO drain up to cap) instead of single-event only
- `Implemented`: controller queue now coalesces consecutive duplicate events/payloads to reduce redundant event churn
- `Partial`: no dedicated controller thread or full C++ event queue semantics

### FFI layer (`avplayer_ffmpeg_ffi.elisa`)

- `Implemented`: container open/close, stream count/info, packet read-based timestamp pulls
- `Implemented`: decoder discovery/open, codec-parameter copy, packet/frame lifecycle helpers
- `Implemented`: FFmpeg-backed timestamp decode helper and stream-language lookup
- `Implemented`: stream-specific packet timestamp read helper (`avplayer_ffmpeg_read_stream_packet_ts`)
- `Implemented`: decoder EOF drain attempt (flush packet + receive) before signaling terminal EOF
- `Implemented`: seek/flush accessors for source-level replay handling
- `Implemented`: ABI prefix layout checks for core structs
- `Missing`: swscale/swresample conversion helpers and full frame-format conversion parity

## Remaining Gaps To Reach 100% C++ Parity

- Port full source pipeline behaviors from `avplayer_source.cpp`:
  demux loop, decoder loops, frame buffering, and frame conversion metadata parity.
- Port controller-thread event processing behavior from `avplayer_state.cpp`.
- Extend FFmpeg FFI to cover swscale/swresample conversion APIs and
  validate any new layout prefixes with `c-bind-check`.
- Expand parity harness scenarios for EOF/loop, buffering transitions, and
  invalid-state edge behavior.
