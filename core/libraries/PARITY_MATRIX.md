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

## audio / audio3d / ajm
- Existing parity suites currently passing for covered behavior: `Exact`
- Functions intentionally returning success/no-op in the C++ surface remain mirrored: `Exact` (stub parity where applicable)

## Deferred / outstanding
- None currently identified in covered core/libraries parity suites and emit gates.

## Validation gates
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_port_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_audio_parity_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_audio3d_parity_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_ajm_instance_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_ajm_batch_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/core_libraries_ajm_aac_tests.elisa`
