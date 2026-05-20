# video_core C++ vs Elisa Parity Matrix

Scope covered:
- `cache_storage.cpp/.h`
- `page_manager.cpp/.h`
- `renderdoc.cpp/.h`

Status: `Exact` for tracked contract-level behavior.

Validation gate:
- `go run ./src ../elisa-shad-ps4-from-scratch/video_core_renderer_vulkan_parity_tests.elisa`
