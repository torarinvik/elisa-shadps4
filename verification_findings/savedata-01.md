Minimal repro:
```elisa
law FitsAt(self: usize, need: usize, cap: usize) = self + need <= cap
struct AbiView:
    base: void&
    size: usize
def read(v: AbiView, off: usize) -> u32:
    requires off + 4 <= v.size
    return 0
def caller(p: void&) -> u32:
    v: AbiView = AbiView{base: p, size: 64}
    return read(v, 0)
```
Note:
1. `elisac -strict --explain -emit semantic` does not discharge `off + 4 <= v.size` through the `AbiView{size: 64}` field.
2. The save-data ABI patch keeps the size-carrying view and guards raw offset access in the helper body instead.
3. Full save-data verification is also blocked by existing non-owned `core/file_format/binary.elisa` / `psf.elisa` strict proof residuals.

---
## RESOLVED (compiler fix, 2026-06-22)
The prover now discharges `requires off + N <= v.size` through a struct-literal field constant.
Fix: struct-literal locals record their field constants (`writtenStruct`, routed through the same
invalidation as `writtenConst`), and `affineOf`/`substitutedAffine` gained a `FieldExpr` case that
resolves `v.field` to that constant. Sound in both polarities (verified: a violating call is still
rejected; a field write / mutable borrow drops the fact). Regression test:
compiler/src/semantic/refinement_struct_field_test.go. The AbiView pattern from Task 1 now proves
statically (zero release cost) instead of falling back to in-body guards.
