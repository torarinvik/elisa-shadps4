# core/libraries C++ vs Elisa Parity Matrix

Status values:
- `Exact`: behavior intended to match current C++ behavior.
- `Intentional Divergence`: Elisa behavior differs by design and is documented.
- `Deferred`: known gap not yet completed.

## app_content
- Core entitlement table behavior (`GetAddcontInfo`, key lookup, mount mapping): `Exact`
- Loader/debug hooks used by emulator integration: `Exact`
- Stubbed APIs returning success as in C++ surface: `Exact` (stub parity)

## avplayer
- Handle lifecycle + state transitions: `Exact`
- Stream enable/disable/start/stop/pause/resume contracts: `Exact`
- Stream-info metadata in FFmpeg path: `Exact`
  - Stream type and codec-parameter metadata (video width/height, audio channels/sample-rate) are read from FFmpeg `codecpar` fields with ABI-prefix-safe bindings.
- Subtitle/timed-text stream selection during auto-start: `Exact`
- Error-code contracts for invalid params/handles: `Exact`
- Playback-clock bookkeeping for `CurrentTime`: `Partial`
  - The Elisa port tracks media timestamps and explicit jumps, but it still does not model the full runtime clock/pause accumulator behavior from C++.

## content_export
- Init/init2/term contracts and reserved/buffer constraints: `Exact`
- Multi-init/noinit handling and error paths: `Exact`

## move
- Init/open/close/term lifecycle contracts: `Exact`
- Stubbed motion state and extension behavior (`sceMoveReadStateLatest`, `sceMoveReadStateRecent`, `sceMoveGetExtensionPortInfo`) returns: `Exact`
- Error contracts for not-initialized and invalid arguments: `Exact`

## audio / audio3d / ajm
- Existing parity suites currently passing for covered behavior: `Exact`
- Functions intentionally returning success/no-op in the C++ surface remain mirrored: `Exact` (stub parity where applicable)

## system (userservice / systemservice / commondialog / msgdialog)
- Stateful common-dialog/msg-dialog lifecycle and error contracts: `Exact` (covered subset)
- User-service event/user lookup behaviors in pure Elisa model: `Exact` (covered subset)
- Runtime-coupled host behaviors (`LoadExec`, host user manager details): `Deferred`
  - Optional FFI bridge contracts added in:
    - `core/libraries/system/userservice_ffi_bridge.elisa`
    - `core/libraries/system/systemservice_ffi_bridge.elisa`
  - Integration notes:
    - `core/libraries/system/FFI_INTEGRATION.md`

## random
- `sceRandomGetRandomNumber` size gating and byte-fill behavior: `Exact`
- Export registration (`libSceRandom`): `Exact`

## razor_cpu
- Full `sceRazorCpu*` surface currently stubbed in C++: `Exact` (stub parity)
- Bool-return contracts (`sceRazorCpuFlushOccurred`, `sceRazorCpuIsCapturing`): `Exact`

## rtc
- Core validation and date/time utility contracts (`sceRtcCheckValid`, leap/year/month/day): `Exact`
- Tick/date, Unix time, Win32 file time, DOS time, and add/subtract contracts: `Exact`
- RFC2822/RFC3339 parser and formatter contracts: `Exact`
- Local timezone/current clock behavior via native C time APIs: `Exact`

## videodec / videodec2
- Resource query sizing and memory alignment contracts: `Exact`
- Decoder/compute queue handle lifecycle and argument/struct-size errors: `Exact`
- Empty decoder queue decode/flush/reset state behavior: `Exact`
- H.264 frame decode payload generation: `Intentional Divergence`
  - `videodec.elisa` preserves observable validation and no-frame state contracts without embedding the FFmpeg-backed C++ decoder.
- Default runtime path (`videodec_native.elisa`): `Exact`
  - `videodec_native.elisa` is now a compatibility include for the pure Elisa implementation; the FFmpeg-backed C++ bridge has been removed from `project.json`.

## Deferred / outstanding
- None currently identified in covered core/libraries parity suites and emit gates.

## Validation gates
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_port_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_audio_parity_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_audio3d_parity_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_ajm_instance_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_ajm_batch_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_ajm_aac_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_usbd_parity_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_move_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_ngs2_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_random_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_razor_cpu_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_rtc_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_videodec_pure_tests.elisa`
- `go run ./src test videodec-native-link-smoke --project ../elisa-shad-ps4-from-scratch`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_share_play_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_signin_dialog_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_system_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_libraries_system_ffi_bridge_tests.elisa`
