# Pad Native Bridge Plan

Use a C++ bridge with a C ABI, not direct raw calls into C++ symbols.

## Why

- `pad.cpp` depends on C++ runtime state: user management, controller objects, emulator settings, touch/orientation history, lightbar state, and vibration backends.
- Elisa can call C ABI functions reliably through `extern`, `@link_name`, and `@c_abi(c)`.
- A small `extern "C"` bridge keeps C++ ownership and object lifetimes inside C++, while giving Elisa stable function names.

## Shape

- `pad.elisa` remains the pure deterministic fallback used by isolated tests and bootstrap builds.
- `pad_ffi.elisa` declares `c_pad_*` externs for native-backed calls.
- `pad_elisa_bridge.cc` wraps the real `Libraries::Pad::scePad*` functions.
- A later build target links `pad_elisa_bridge.cc` only when the full shadPS4 C++ runtime is available.

## Next Migration Steps

1. Replace simplified Elisa pad structs with `@c_bind` layouts, or introduce explicit bridge DTOs.
2. Add a native integration test target that links `pad_elisa_bridge.cc`.
3. Route backend-dependent functions through `c_pad_*` in native builds.
4. Keep pure tests pinned to the fallback behavior so compiler-only development stays fast.
