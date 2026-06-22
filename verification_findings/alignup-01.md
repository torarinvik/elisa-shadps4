# AlignUp no-overflow `requires` + call-site guards

## Goal
Add a no-overflow `requires` to the two AlignUp functions and guard every call
site so the `ensure`s discharge (with paired compiler change **C1**), stopping the
`Module_AlignUp` overflow from making `module.elisa`-importing targets red
(`core-libraries-pad-tests`, `core-libraries-audio-parity-tests`).

## Requires added (first body statement, before the `ensure`s)
- `core/module.elisa` `Module_AlignUp` (line ~709):
  `requires value + alignment <= 0xFFFFFFFFFFFFFFFF`
- `core/libraries/kernel/memory.elisa` `KernelMemory_AlignUp` (line ~46):
  `requires value + alignment <= 0xFFFFFFFFFFFFFFFF`

## Per-call-site guards
| Site | Location | Guard |
|------|----------|-------|
| M1/M2/M3/M4 | `module.elisa` `Module_GetAlignedProgramHeaderSize` | single guard in the shared wrapper: `if phdr.p_memsz > 0xFF..FF - phdr.p_align: return phdr.p_memsz` (p_align is an unbounded attacker-controlled ELF field). All four routes (`Module_GetAlignedProgramHeaderSize` at GetAlignedSize, MapImageSegment, ProtectImageSegment, ComputeAlignedImageSize) go through this wrapper, so one guard discharges them. |
| M5 | `module.elisa` `Module_PrepareLoadImage` (`Module_AlignUp(base_size, MODULE_BLOCK_ALIGN)`) | `if base_size > 0xFF..FF - MODULE_BLOCK_ALIGN: base_size <- 0xFF..FF - MODULE_BLOCK_ALIGN` (base_size is an accumulated max-end, not capped). |
| K1 | `kernel/memory.elisa` `KernelMemory_PosixMmapImpl` vaddr | `if vaddr > 0xFF..FF - KERNEL_MEMORY_PAGE_SIZE: return MMAP_FAILED` (guest-supplied addr). |
| K2 | same fn, len | `if len > 0xFF..FF - KERNEL_MEMORY_PAGE_SIZE: return MMAP_FAILED` (only len==0 was checked before). |
| K3 | `kernel/memory.elisa` `sceKernelMprotect` | `if not Module_U64AddNoOverflow(size, in_addr): return EINVAL` then bind `span = size+in_addr-aligned_addr` and `if span > 0xFF..FF - PAGE_SIZE: return EINVAL`. The subtraction `in_addr - aligned_addr` is in `[0, page_size)` because `aligned_addr = AlignDown(in_addr)`, so guarding the two additions suffices. |
| L1 | `linker.elisa` `Linker_AllocateTlsForThread` | `static_tls_size` is a u32 → `.u64() <= 2^32-1`, +0x20 cannot wrap. SAFE by width; the map said "no guard". The installed prover does not track the u32 width through `.u64()`, so a cheap explicit clamp was added (`tls_size_u64` mutable + clamp) to make the bound visible. |
| L2 | `linker.elisa` `Linker_NextModuleBase` | **latent overflow fixed first**: the running `module_end = base_virtual_addr + aligned_base_size + MODULE_TRAMPOLINE_SIZE` accumulation can itself wrap. Rewrote with two `Module_U64AddNoOverflow` checks (skip a module whose end would wrap), then a final `if base > 0xFF..FF - MODULE_BLOCK_ALIGN: base <- ...` clamp before the `Module_AlignUp` call. |

These are real security fixes for the K-sites (guest-/ELF-controlled inputs) and a
latent-bug fix for L2, independent of the verification benefit.

## Files edited (source only, per constraints)
- `core/module.elisa`
- `core/linker.elisa`
- `core/libraries/kernel/memory.elisa`
No edits to `project.json` / `check_elisa_port.py`.

## C1 dependency — NOT yet dischargeable with the installed compiler
The installed `elisac` does **not** honor the new `requires` as a body assumption,
nor does it propagate the per-site guard facts into the call-site precondition
check. Verified: with `-smt` and in `test` mode the prover still reports the
counterexample `alignment=2, value=2^64-1` for `Module_AlignUp`'s
`ensure result >= value`, even though `requires value+alignment <= MAX` excludes it.
This is exactly the requires→ensure / guard-fact→precondition threading that the
paired compiler agent **C1** is implementing.

Additionally, `Module_AlignUp`'s body `((value+alignment-1)/alignment)*alignment >= value`
is **nonlinear** (div + mul); even once the requires is honored it will need the
SMT discharge tier (`-smt`) — the affine prover alone cannot close it.
`KernelMemory_AlignUp`'s `value + (alignment - remainder)` form IS affine and should
close once C1 honors the requires.

## Proof-error counts (installed, non-C1 compiler) — AlignUp ensure+precondition only
Measured via `ELISACORE_TEST_CACHE=0 elisac test <target> --project .`,
filtering `Module_AlignUp`/`KernelMemory_AlignUp` could-not-prove lines:

| Target | BEFORE (original src) | AFTER (this change, pre-C1) | AFTER (expected, post-C1) |
|--------|----------------------:|----------------------------:|--------------------------:|
| core-libraries-pad-tests        | 2 | 6 | 0 |
| core-libraries-audio-parity-tests | 2 | 6 | 0 |
| emulator-guest-exec-runtime-tests | 2 | 6 | 0 |

BEFORE = 2: the two genuine `ensure result >= value` overflow returns (no requires).
AFTER pre-C1 = 6: same 2 ensure returns (requires not yet honored) + 4 new
call-site preconditions the requires introduced (723, 860, linker 2193, 2283 in
module/linker; kernel sites also flag in standalone emit). This temporary increase
is purely the C1 gap; the guards are logically sufficient and will collapse all 6
to 0 once C1 threads the facts. No call site was left unguarded.

## No structural regressions introduced
After fixing one `mutable` slip (`tls_size_u64`), a full
`elisac test core-libraries-pad-tests --project .` shows **no** `error:` /
type-mismatch / unknown-name diagnostics from the three edited files — only the
expected (C1-gated) AlignUp proof obligations remain.
