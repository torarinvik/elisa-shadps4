# CUSA07399 Graphics/Video Fallback Report

Date: 2026-05-21

## Scope

Runtime-first-frame path status for:

- `core/libraries/videoout`
- `core/libraries/gnmdriver`
- `video_core`
- `shader_recompiler`

## libSceVideoOut imports (CUSA-first-frame critical)

- `sceVideoOutOpen`: `Native Elisa` (instrumented)
- `sceVideoOutRegisterBuffers`: `Native Elisa` (instrumented)
- `sceVideoOutSubmitFlip`: `Native Elisa` (instrumented)
- `sceVideoOutGetFlipStatus`: `Native Elisa`
- `sceVideoOutAddFlipEvent`: `Native Elisa`
- `sceVideoOutGetVblankStatus`: `Native Elisa`
- `sceVideoOutWaitVblank`: `Native Elisa`

Current stage artifacts emitted:

- `VIDEO_STAGE_opened`
- `VIDEO_STAGE_buffers_registered`
- `VIDEO_STAGE_flip_submitted`
- `VIDEO_STAGE_flip_completed`
- `VIDEO_STAGE_event_emitted`
- `VIDEO_STAGE_first_frame_candidate`

## GNM/GNMX submit path

- `sceGnmSubmitCommandBuffersForWorkload`: `Native Elisa` with explicit submit-path classification
- `sceGnmSubmitAndFlipCommandBuffersForWorkload`: `Native Elisa` with explicit flip-stage emission
- `sceGnmSubmitDone`: `Native Elisa`

Submit-path classification artifacts:

- `gnm_submit_path_native`
- `gnm_submit_path_bridge`
- `gnm_submit_seen`

Renderer lifecycle artifact:

- `renderer_initialized`

## Shader/compiler-related path

- `ShaderRecompiler_TranslateProgramViaFfi`: `Needs temporary C++ bridge` (stable C ABI exposed, current local bridge returns placeholder exception code)
- `ShaderRecompiler_FreeProgramViaFfi`: `Bridge-backed`

Shader artifact:

- `shader_translate_attempted`

## Bridge policy status

- Guest-facing APIs in this path are exposed in Elisa.
- C++ internals remain bridge-backed where Vulkan/runtime ABI modeling is not yet complete.
- Bridge usage is surfaced explicitly via artifact counters and submit-path keys above.

## Current blocker summary to first real frame

- Core videoout lifecycle path is active and staged.
- GNM submit/submit+flip path is active and staged.
- Renderer init is visible via artifact.
- Shader bridge is currently attempt-visible, not success-visible, in this repo slice.

