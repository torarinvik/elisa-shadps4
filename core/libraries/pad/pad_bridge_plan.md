# Pad Native Bridge Plan

Use a C++ bridge with a C ABI, not direct raw calls into C++ symbols.

## Why

- `pad.cpp` depends on C++ runtime state: user management, controller objects, emulator settings, touch/orientation history, lightbar state, and vibration backends.
- Elisa can call C ABI functions reliably through `extern`, `@link_name`, and `@c_abi(c)`.
- A small `extern "C"` bridge keeps C++ ownership and object lifetimes inside C++, while giving Elisa stable function names.

## Shape

- `pad.elisa` remains the pure deterministic fallback used by isolated tests and bootstrap builds.
- `pad_ffi.elisa` declares `c_pad_*` externs for native-backed calls and verifies bridge DTO layouts through `@c_bind`.
- `pad_elisa_bridge.hh` defines the C-compatible DTO ABI used at the boundary.
- `pad_elisa_bridge.cc` wraps the real `Libraries::Pad::scePad*` functions and translates DTOs to/from C++ pad structs.
- `pad_native.elisa` is the native-backed Elisa surface; it routes backend-dependent exports through `c_pad_*`.
- `native_bridge_project` links a small C test bridge so Elisa-to-C ABI calls are verified without requiring the full shadPS4 runtime.
- A later full-runtime target should link `pad_elisa_bridge.cc` plus the shadPS4 C++ runtime dependencies.

## Current Status

1. Pure fallback tests are pinned to `pad.elisa`.
2. DTO layouts are checked with `c-bind-check` against `pad_elisa_bridge.hh`.
3. The native Elisa wrapper lowers cleanly.
4. The project-local native bridge test links a foreign C stub and verifies lifecycle plus struct output forwarding.
5. `pad.elisa` now consumes shared Elisa controller/input state (`input/controller.elisa`, `input/input_handler.elisa`) for live button/axis, touch, motion, lightbar, and vibration behavior.

## Remaining Integration Step

`pad_elisa_bridge.cc` remains available for builds that still need direct C++ pad/runtime coupling, but it is no longer required for core Elisa pad/input behavior.
