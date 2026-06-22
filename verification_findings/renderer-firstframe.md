# E1 — Renderer first frame: status & blocker report

Date: 2026-06-22

## Headline

A **real first frame is produced and captured to disk** via the videoout submit/flip
lifecycle. Pixels originate in a guest-visible framebuffer, travel through
`sceVideoOutRegisterBuffers` -> `sceVideoOutSubmitFlip` -> the vblank flip pump, and are
recovered from the presenter's recorded `last_present_addr` and serialized to a binary
PPM (P6).

Evidence:
- `verification_findings/first_frame.ppm` — 107 bytes, header `P6\n8 4\n255\n` + 96 bytes
  (8x4x3) RGB body matching the painted gradient.
- `verification_findings/first_frame.png` — same frame, decoded by the host `sips` image
  decoder (independent proof the PPM is a valid image, not just bytes).
- `elisa_tests/emulator_renderer_first_frame_capture.elisa` — passes
  (`passed=1`), asserts the presenter points at the painted buffer and round-trips pixels.

## What changed

- `core/libraries/videoout/videoout.elisa`
  - Presenter accessors: `videoout_present_count`, `videoout_last_present_addr/width/height/format`.
  - `videoout_capture_present_ppm(out)` — reads the last-flipped guest framebuffer
    (A8R8G8B8 / A8B8G8R8 SRGB, 32bpp) and serializes a P6 PPM. Black/blank flips
    (addr==0) and non-32bpp formats return empty (no fabricated frame).
  - PPM byte-emit helpers `_videoout_ppm_push_cstr`, `_videoout_ppm_push_u32`.
- `elisa_tests/emulator_renderer_first_frame_capture.elisa` — new capture test.
- `verification_findings/PENDING_TARGETS/E1.md` — project.json target stanza (per
  constraint: not editing project.json directly).

No edits to `project.json` or `check_elisa_port.py`. Compiler untouched (used read-only).

## Shader bridge status (the `ELISA_SHADER_BRIDGE_REAL_SPIRV` path)

The `CUSA07399_GRAPHICS_FALLBACK_REPORT.md` description of an env-gated real-SPIR-V
emitter running in a POSIX child process (a C++ bridge with process isolation) is
**stale relative to the current tree**. That C++ bridge
(`recompiler_elisa_bridge.cpp`) has been replaced by a **pure-Elisa** recompiler:
`shader_recompiler/recompiler_translate.elisa` (included via
`recompiler_ffi_bridge.elisa`).

What the pure-Elisa path emits today:
- A deterministic **12-word SPIR-V envelope** (real SPIR-V magic `0x07230203` in word 0,
  version/generator/bound, then metadata words: stage, logical stage, input-hash halves,
  instruction/block/syntax counts). `payload_kind = SPIRV_ENVELOPE`,
  `real_spirv_status = NOT_REQUESTED`.
- This is a stable metadata header, **not a full compiled SPIR-V module**. There is no
  live code path in the current tree that sets `payload_kind = REAL_SPIRV` or
  `real_spirv_status = SUCCEEDED`. `grep -rln ELISA_SHADER_BRIDGE_REAL_SPIRV` finds the
  string only in the (stale) markdown report — it is referenced by **no** `.elisa` source.

### Precise blocker to *real SPIR-V emission* (distinct from the first frame)

- Missing: an Elisa code path that consumes the decoded GCN program
  (`shader_recompiler/frontend` + `ir`) and lowers it to a real SPIR-V module, then sets
  `output_words`/`output_word_count`/`payload_kind = REAL_SPIRV`. The frontend
  (decode -> CFG -> ASL) exists and is exercised by
  `shader_recompiler_frontend_pure_tests.elisa`, but it is **not wired into**
  `elisa_shader_recompiler_translate_program` — that function stops at the envelope.
- The `ElisaRecompilerProgramHandle.output_words` field is a fixed `u32[12]`, so the
  storage itself caps at the 12-word envelope; real emission needs a variable-length
  output buffer (heap/region-backed darray) before a full module can be stored.

This blocker does **not** gate the first frame: the videoout flip path presents a guest
framebuffer regardless of shader payload contents, which is what the capture proves. Real
SPIR-V is required for *content rendered by GCN shaders* (a later milestone), not for the
flip-and-present first-frame milestone.

## How to reproduce

```
ELISACORE_TEST_CACHE=0 elisac -emit test elisa_tests/emulator_renderer_first_frame_capture.elisa
ELISACORE_TEST_CACHE=0 elisac -emit test elisa_tests/emulator_renderer_first_frame_smoke.elisa
sips -s format png verification_findings/first_frame.ppm --out /tmp/ff.png   # host decode check
```
