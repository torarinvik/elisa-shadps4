# renderer_vulkan C++ vs Elisa Parity Matrix

Status values:
- `Exact`: Elisa behavior mirrors the tracked C++ contract-level behavior.
- `Deferred`: known parity gap still open.

## Core backend parity

- `liverpool_to_vk.cpp/.h` enum and format translation paths: `Exact`
- `vk_common.cpp/.h` Vulkan shared constants and handle helpers: `Exact`
- `vk_compute_pipeline.cpp/.h` compute pipeline key/layout creation flow: `Exact`
- `vk_graphics_pipeline.cpp/.h` graphics key + descriptor layout + vertex-input contract: `Exact`
- `vk_instance.cpp/.h` device capability/model queries and format fallback logic: `Exact`
- `vk_master_semaphore.cpp/.h` timeline tick accounting + refresh/wait behavior: `Exact`
- `vk_pipeline_cache.cpp/.h` graphics/compute pipeline cache warmup/build/sync lifecycle: `Exact`
- `vk_pipeline_common.cpp/.h` resource binding and descriptor push/commit policy: `Exact`
- `vk_pipeline_serialization.cpp/.h` pipeline/shader metadata registration contract: `Exact`
- `vk_platform.cpp/.h` instance layer/extension selection and safe-mode toggles: `Exact`
- `vk_presenter.cpp/.h` frame selection, swapchain present, screenshot consumption hooks: `Exact`
- `vk_rasterizer.cpp/.h` draw/dispatch submission paths and mapped memory tracking: `Exact`
- `vk_resource_pool.cpp/.h` recyclable resource commit + descriptor heap behavior: `Exact`
- `vk_scheduler.cpp/.h` submission timeline and dynamic-state commit flow: `Exact`
- `vk_shader_hle.cpp/.h` copy-shader HLE detection/dispatch: `Exact`
- `vk_shader_util.cpp/.h` GLSL/SPIR-V module compilation contract surface: `Exact`
- `vk_swapchain.cpp/.h` create/recreate/acquire/present lifecycle: `Exact`

## Validation gate

- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/video_core_renderer_vulkan_parity_tests.elisa`

## Deferred

- None currently tracked for this scope.
