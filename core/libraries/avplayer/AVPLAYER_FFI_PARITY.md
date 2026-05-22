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
- `Implemented`: `Start` now matches C++ restart semantics for already-active playback by stopping the source first, publishing `Stop`, then entering `Starting`/`Play` again.
- `Implemented`: `Stop` now follows the C++ ordering: source/workers are stopped before the public `Stop` state/event is emitted.
- `Implemented`: `Pause` is a success no-op when the player is already at EOF, matching `AvPlayerState::Pause`.
- `Implemented`: constructor/init autoplay behavior now matches C++: missing guest event callback or explicit `auto_start` enables internal autoplay.
- `Implemented`: internal autoplay consumes `StateReady` and forwards later state events such as `StatePlay` to the guest callback, matching C++ `AutoPlayEventCallback`.
- `Implemented`: stream count/info, get audio/video data, loop flag, current time
- `Implemented`: `SetAvSyncMode` now stores sync mode in source state; `None` bypasses video delivery sync gating like C++ source behavior
- `Implemented`: `SetLooping` now updates both source and controller loop state and fails when no source is attached
- `Implemented`: `JumpToTime` matches current C++ stub behavior (`valid handle -> ORBIS_OK`, no source mutation/event side effects)
- `Implemented`: `DisableStream` matches current C++ stub behavior (`valid handle -> ORBIS_OK`, no source mutation side effects)
- `Implemented`: `SetTrickSpeed` matches current C++ stub behavior (`valid handle -> ORBIS_OK`)
- `Implemented`: `SetLogCallback` matches current C++ stub behavior (`ORBIS_OK` no-op)
- `Implemented`: subtitle/timed-text streams can participate in auto-start and startup validation
- `Implemented`: handle validation and basic error returns
- `Implemented`: timing uses a monotonic playback clock for `CurrentTime`
- `Partial`: state/event ordering and buffering behavior are simplified vs C++

### Source runtime (`avplayer_source.elisa`)

- `Implemented`: FFmpeg open/close/start/stop/pause/resume calls
- `Implemented`: start path seeks streams back to the beginning before decoder setup
- `Implemented`: restarted playback reopens codec contexts and resumes decode-backed frame retrieval after the C++ `Start` restart path.
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
- `Implemented`: loop restart now surfaces `ORBIS_AVPLAYER_ERROR_WAR_LOOPING_BACK` through the controller warning path, matching the C++ observable warning behavior
- `Implemented`: bounded decode-driven output fill step before get-audio/get-video return paths
- `Implemented`: source-side cached stream metadata (video dimensions, audio channel/sample rate) used by frame getters with stream-info fallback
- `Implemented`: frame getters now clear stream detail payloads before filling (prevents stale metadata leakage across calls)
- `Implemented`: `AvPlayerFrameInfoEx` now uses a richer Ex-specific detail layout closer to the C++ public API, including video framerate/crop/pitch/bit-depth fields
- `Implemented`: `GetVideoDataEx` now deterministically fills Ex-only video metadata defaults (framerate fallback, crop offsets, pitch, bit-depth, range flag) from current source/cadence state
- `Implemented`: Ex-detail clear path now zeroes all reserved Ex payload bytes (`audio/video/subs`) to better mirror C++ union-style zero-initialization behavior
- `Implemented`: source tracks delivered frame interval estimates; audio frame `size` now uses cadence-based estimate instead of constant zero
- `Implemented`: source now caches delivered audio frame byte-size per output frame; API consumes delivered size first and only falls back to estimate when unavailable
- `Implemented`: FFmpeg decode path now exposes decoded audio frame metadata (`nb_samples`, `sample_rate`, `channels`) and source uses it to set frame-accurate audio byte-size when available
- `Implemented`: FFmpeg decode path now exposes decoded video frame metadata candidates (`width`, `height`, `linesize[0]`, aspect numerator/denominator) and `GetVideoDataEx` prefers delivered aspect/pitch when available
- `Implemented`: non-Ex `GetVideoData` now also prefers delivered decode aspect ratio (num/den) when available, reducing fallback-only aspect behavior
- `Implemented`: `GetVideoDataEx` `video_full_range_flag` now reads from source-side delivered metadata cache (plumbed path ready for decode-fed color-range)
- `Implemented`: source now initializes video full-range cache from FFmpeg stream `color_range` metadata (full-range when FFmpeg reports JPEG/full-range), and `GetVideoDataEx` consumes it
- `Implemented`: lifecycle resets (`init/add-source/stop`) now clear decode-driven video metadata caches (pitch/aspect/full-range); seek/jump clears per-frame pitch/aspect while preserving stream-level full-range metadata
- `Implemented`: FFmpeg 8 `AVFrame` prefix binding now follows the public `data[8]` layout and validates with `c-bind-check`
- `Implemented`: successful `GetAudioData`, `GetVideoData`, and `GetVideoDataEx` now allocate stable frame payload buffers through the player memory callbacks instead of returning metadata-only frames with `p_data = null`
- `Implemented`: FFmpeg decode now has an Elisa-side copy path that copies live decoded frame data into caller-provided payload buffers before `AVFrame` release
- `Implemented`: FFmpeg fixture tests now verify real MP4 video and audio decode into caller-owned payload buffers, not just metadata/timestamp decode
- `Implemented`: callback-backed file replacement now tracks open fd, file size, position, read-packet EOF/short-read behavior, seek/reset clamping, close-on-replace, and close-on-release in Elisa
- `Implemented`: decoded payload copying routes non-NV12 video through swscale to NV12 and non-S16 audio through swresample to S16 before guest-buffer copy
- `Partial`: swscale/swresample conversion helpers are wired and unit-covered for synthetic audio conversion, but still need non-native-format media fixture coverage for video and audio conversion payload bytes
- `Different`: Elisa currently cannot model nullable function-pointer fields directly, so `AvPlayerMemAllocator` callback fields are represented as callable function values; this preserves callback invocation and function-pointer-sized ABI shape, but null-callback validation is deferred to compiler nullable-function support
- `Implemented`: source caches stream start-time/duration for active audio/video and frame getters now apply cached stream start-time offset to output timestamps
- `Implemented`: start-time offset application now keys on `start_time` presence (not duration presence), improving streams with unknown/zero duration
- `Implemented`: handle `CurrentTime` bookkeeping now updates from post-offset frame timestamps (internal time matches returned timeline)
- `Implemented`: `CurrentTime` now prefers media-clock timestamps (audio/video decoded output) before wall-clock fallback
- `Implemented`: media-clock `CurrentTime` now includes cached stream start-time offsets for timeline-aligned reporting
- `Implemented`: buffering state updates in frame getters are now source-readiness driven (`avsrc_has_frames`) instead of hardcoded booleans
- `Implemented`: source readiness now treats demux packet queues as frame availability signals (not just decoded output queues)
- `Implemented`: source readiness now only counts queues for enabled stream types (avoids stale-queue false positives after stream mode changes)
- `Implemented`: EOF signaling from frame getters is now unified across audio/video paths using source activity (`avsrc_is_active`)
- `Implemented`: frame getters now only emit controller EOF when source EOF is terminal; loop-enabled restart windows no longer raise false EOF transitions
- `Implemented`: source activity (`IsActive`) now includes demux packet queues, preventing false-inactive states while decode work is buffered
- `Implemented`: source activity queue checks are now filtered by enabled stream types (avoids stale-queue false-active states after stream mode changes)
- `Implemented`: source stream-enable/disable internals are present for direct source tests, while public `sceAvPlayerDisableStream` intentionally remains C++-stub-compatible no-op at API layer
- `Implemented`: `CurrentTime` wall-clock behavior matches C++ source timing model (continues advancing while paused until resume applies accumulated pause duration)
- `Implemented`: add-source paths now reset queue state, stream metadata/timing caches, cadence markers, and playback time anchors before new source lifecycle begins
- `Implemented`: add-source source-runtime baseline now also resets loop mode to false for deterministic per-source setup
- `Implemented`: add-source source-runtime baseline now nulls decoder context pointers before next start-time decoder initialization
- `Implemented`: init/initEx now clamp requested output video framebuffer count to the C++ range `[2, 16]` and store it in source state
- `Implemented`: `IsActive` and `CurrentTime` gating now follow the C++ source shape
- `Partial`: no full demux/decode threads, packet queues, or converted frame pipeline
- `Implemented`: guest payload buffers now use allocator-backed per-handle ring pools (video pool count tracks requested framebuffer count, audio uses rotating pool) and are released on close

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
- `Implemented`: controller flushes and scrubs pending queued events on hard transitions (`STOP`, explicit `ERROR`, add-source failure to `ERROR`, and `REVERT_STATE`)
- `Implemented`: add-source entry now hard-flushes queued controller events before scheduling add-source processing
- `Implemented`: add-source controller flow now re-baselines state to `INITIAL` before entering `ADDING_SOURCE`
- `Implemented`: warning event payload forwarding is parity-tested for source-side loop-back warning paths
- `Implemented`: buffering transition event ordering (`PLAY -> BUFFERING -> PLAY`) is parity-tested through controller tick dispatch
- `Implemented`: controller dispatch now skips callback invocation when the guest did not install an event callback, matching the C++ no-callback behavior instead of calling a dummy Elisa function pointer
- `Implemented`: controller tick now forwards pending source warnings and queues EOF transitions before dispatching guest events, matching C++ controller signaling paths without introducing unsynchronized source-thread races.
- `Implemented`: started FFmpeg-backed public frame retrieval now opens source codecs, uses a concrete libc clock FFI, enters active playback, and returns decoded video/audio payloads through `sceAvPlayerGetVideoDataEx` and `sceAvPlayerGetAudioData`
- `Implemented`: `Stop -> Starting -> Play` is now a valid transition so explicit restart from stopped/active paths follows C++ `AvPlayerState::Start`.
- `Implemented`: init starts a host-backed controller tick thread, mirroring C++ constructor lifecycle.
- `Partial`: controller thread is currently limited to event/buffering ticks; full C++ source worker-thread demux/decode queues are still pending, so background frame decoding is intentionally not performed from the controller thread.

### FFI layer (`avplayer_ffmpeg_ffi.elisa`)

- `Implemented`: container open/close, stream count/info, packet read-based timestamp pulls
- `Implemented`: decoder discovery/open, codec-parameter copy, packet/frame lifecycle helpers
- `Implemented`: FFmpeg-backed timestamp decode helper and stream-language lookup
- `Implemented`: stream-specific packet timestamp read helper (`avplayer_ffmpeg_read_stream_packet_ts`)
- `Implemented`: decoder EOF drain attempt (flush packet + receive) before signaling terminal EOF
- `Implemented`: seek/flush accessors for source-level replay handling
- `Implemented`: ABI prefix layout checks for core structs
- `Implemented`: swscale/swresample declarations and conversion helper paths are present for non-NV12 video and non-S16 audio; binding checks and runtime unit tests cover conversion behavior, including aligned NV12 destination payload layout

## Remaining Gaps To Reach 100% C++ Parity

- Port full source pipeline behaviors from `avplayer_source.cpp`:
  demux loop, decoder loops, frame buffering, and frame conversion metadata parity.
- Port controller-thread event processing behavior from `avplayer_state.cpp`.
- Add media-fixture runtime tests for swscale/swresample conversion outputs using files that are not already NV12/S16.
- Expand parity harness scenarios for EOF/loop, buffering transitions, and
  invalid-state edge behavior.
- `Implemented`: API validation matrix expanded for invalid handles and C++-stub control entrypoint contracts.
- `Implemented`: `sceAvPlayerIsActive` now follows C++ activity semantics for paused playback (`Pause` remains active while source/state are active).
- `Implemented`: `AvPlayer_RegisterLib` now registers the full C++ `libSceAvPlayer` NID set (excluding the intentionally non-registered printf vararg entry, matching C++).
