# Devtools Widget Extern Ledger

This ledger classifies C ABI boundaries used by `core/devtools/widget`.

## Allowed External C++ Shims

- `imgui_c_api.h` / `imgui_c_api.cpp`
  - Wraps Dear ImGui, which is an external C++ UI library.
  - This is an allowed long-term C ABI boundary.
  - Elisa should call only the C-compatible surface in `imgui_abi.elisa`.

## Temporary Project-Owned C++ Bridges

- `Core::Memory` snapshot bridge for `memory_map.elisa`
  - This is a deliberate cheat, not an external dependency boundary.
  - Upstream `MemoryMapViewer` reads project-owned `Core::Memory::vma_map`,
    `dmem_map`, and `mutex` through friendship in `src/core/memory.h`.
  - A C API may be used temporarily to expose POD memory-map rows to Elisa,
    because `Core::Memory` has not been ported yet and its internals are C++
    maps, strings, and locks.
  - The C API must not grow widget behavior. It may only lock, snapshot, and
    copy project-owned memory state into C-compatible structs.
  - Once `Core::Memory` / `MemoryManager` is ported to Elisa, this bridge must
    be removed and `memory_map.elisa` should iterate Elisa-owned memory maps
    directly.

Completion rule: C++ shims may remain only for external libraries such as
ImGui. Any bridge whose implementation depends on project-owned shadPS4 C++
logic is temporary and must be tracked here until replaced by Elisa code.

## Removal Checklist For Core::Memory Cheat Bridge

- Port `Core::Memory` map ownership (`vma_map`, `dmem_map`, mutex lifecycle)
  to Elisa-owned data structures.
- Replace `elisa_devtools_memory_map_snapshot_vma/dmem` calls in
  `memory_map.elisa` with direct Elisa iteration over the Elisa-owned maps.
- Delete `core/devtools/widget/memory_map.cpp` + `memory_map.h` bridge files.
- Keep only external-library shims (for example ImGui) after this removal.
