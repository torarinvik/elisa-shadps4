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
- `Implemented`: `IsActive` and `CurrentTime` gating now follow the C++ source shape
- `Partial`: no full demux/decode threads, packet queues, or converted frame pipeline
- `Partial`: no guest buffer pool or frame-object lifetime model like C++

### State/controller (`avplayer_state.elisa`)

- `Implemented`: state enum model, transition checks, add-source/warning/error events
- `Implemented`: basic buffering transition helper and EOF/error state hooks
- `Implemented`: revert-state event type is now represented in the Elisa controller state
- `Partial`: no dedicated controller thread or full C++ event queue semantics

### FFI layer (`avplayer_ffmpeg_ffi.elisa`)

- `Implemented`: container open/close, stream count/info, packet read-based timestamp pulls
- `Implemented`: decoder discovery/open, codec-parameter copy, packet/frame lifecycle helpers
- `Implemented`: FFmpeg-backed timestamp decode helper and stream-language lookup
- `Implemented`: stream-specific packet timestamp read helper (`avplayer_ffmpeg_read_stream_packet_ts`)
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
