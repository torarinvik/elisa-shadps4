# Memory Map C-API Cheat Boundary

This widget currently uses two C ABI functions from
`core/devtools/widget/memory_map.cpp`:

- `elisa_devtools_memory_map_snapshot_vma`
- `elisa_devtools_memory_map_snapshot_dmem`

These are **temporary project-owned bridge functions**, not a legitimate
long-term external dependency boundary (unlike ImGui).

Why this exists:

- Upstream widget memory-map rendering reads `Core::Memory` internals.
- `Core::Memory` is still C++ in this repo slice.
- The bridge snapshots rows for Elisa UI code until `Core::Memory` is ported.

Parity policy impact:

- This bridge is tracked in `parity_ledger.json` as `Missing` for 100% parity.
- It should be removed once `Core::Memory`/memory manager behavior is ported to Elisa.
- After that port, iterate Elisa-owned memory-map structures directly from
  `core/devtools/widget/memory_map.elisa`.
