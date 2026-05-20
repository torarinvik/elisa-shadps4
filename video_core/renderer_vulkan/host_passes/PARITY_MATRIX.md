# renderer_vulkan/host_passes C++ vs Elisa Parity Matrix

Status values:
- `Exact`: Elisa behavior mirrors current C++ behavior for this path.
- `Exact (C++ stub)`: C++ is intentionally stubbed and Elisa mirrors stub behavior.
- `Deferred`: known parity gap still open.

## `FsrPass` parity

- `Create` resource lifecycle wiring: `Exact`
- `Render` early-return gates (`enable`, upscaling-only): `Exact`
- Strict render validation contract (`SHADPS4_STRICT_RENDER_VALIDATION`): `Exact`
- Ring-buffer image rotation and resize invalidation: `Exact`
- EASU-only path (`use_rcas = false`): `Exact`
- EASU+RCAS path (`use_rcas = true`): `Exact`
- Dispatch sizing (`ceil(output / 16)`): `Exact`
- Host markers (`IsVkHostMarkersEnabled`) begin/end ordering: `Exact`

## `PostProcessingPass` parity

- `Create` pipeline/sampler contract surface: `Exact`
- Strict render validation input/frame checks: `Exact`
- Force-color path (`SHADPS4_FORCE_PP_COLOR`) clear+barrier+return: `Exact`
- Normal draw path (bind/set/push/draw/barrier ordering): `Exact`
- Host markers begin/end ordering: `Exact`

## Validation gates

- `go run ./src ../elisa-shad-ps4-from-scratch/video_core_renderer_vulkan_host_passes_pure_tests.elisa`

## Deferred

- None currently tracked for `host_passes` scope.
