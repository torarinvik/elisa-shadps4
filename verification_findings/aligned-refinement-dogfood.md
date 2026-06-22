# Dogfood: Aligned[N] / Fits[bits] modular refinements on the emulator

The new built-in modular refinement predicates — `Aligned[N]`, `Divides[N]`, `Fits[bits]`,
`MaskedZero[mask]` — were dogfooded against the emulator's page-alignment / register-field
needs. This is the feature that reopens the **E6 design note**: page-alignment had to be an
*operational guard* because `(addr & 0xFFF) == 0` was an undischargeable modular `requires`.
The vocabulary now discharges it — at a cheap syntactic tier for the common shapes (mask /
multiple), falling back to the SMT bitvector lane otherwise.

## What landed (sound, non-vacuous)
`elisa_tests/aligned_refinement_tests.elisa` — **return-position** proven constructors:
- `page_align_down(raw) -> u64 is Aligned[4096]` (`raw & PAGE_MASK`) — proves at the cheap
  tier (low 12 bits cleared), no SMT.
- `reg_field6(reg) -> u32 is Fits[6]` (`(reg >> 12) & 0x3F`) — proves the extraction fits 6 bits.

Both discharge under `-strict -smt` (EXIT 0); the test target is green. **Verified non-vacuous:**
- `-> u64 is Aligned[4096]` over `return raw + 1` is REJECTED (counterexample `raw=0`).
- `-> u32 is Fits[6]` over `(reg >> 12) & 0x7F` is REJECTED (7-bit mask can't fit 6 bits).

This is a real win: a constructor's refined return type lets every downstream consumer treat
the value as statically aligned / width-bounded — no runtime guard, no bounds check.

## FINDING: the parameter-position story has gaps (blocks the highest-value emulator use)
The most valuable emulator use is stating alignment as a **precondition** on the mapping
primitives — `def map_fixed(addr: PageAddr, len: PageAddr)` — so callers must prove alignment.
Three concrete gaps currently block that:

1. **Alias-typed parameter is VACUOUS.** With `type PageAddr = u64 is Aligned[4096]` and
   `def map_fixed(addr: PageAddr, ...)`, passing a plainly **unaligned** `raw` is accepted
   under `-strict` (no error). The refinement erases at the parameter boundary, so the
   precondition is not enforced at call sites. (Matches the implementing agent's own note:
   "a refinement via a type alias erases on return and parameter positions.")
2. **Inline-`is` parameter emits spurious errors.** `def map_fixed(addr: u64 is Aligned[4096], ...)`
   *does* enforce the call-argument obligation, but also reports `undefined identifier "Aligned"`
   + `cannot call non-function value` on the parameter type itself — so it can't be used as-is.
3. **Var-decl inline refinement does not parse.** `aligned: u64 is Aligned[4096] = raw & PAGE_MASK`
   is a parse error (`unexpected token aligned in expression`), and a refinement is also lost
   through a plain untyped local (`base: u64 = page_align_down(raw)` drops the `Aligned` fact).

### Recommended follow-up for the refinement system (A)
- Make alias-typed refinements **enforced** at parameter (and return) positions — the alias
  should be transparent to the discharge ladder, not erase the predicate. This is the single
  highest-value fix: it unlocks `map_fixed(addr: PageAddr)`-style preconditions.
- Fix the inline-`is`-in-parameter resolution so it doesn't emit `undefined identifier`.
- Support inline refinement on `var` declarations, and propagate a refined return type into an
  untyped local binding (so `base := page_align_down(raw)` keeps `Aligned[4096]`).
- Additive congruence depth: `aligned_param + (pages << 12)` (a *variable* multiple of N) is
  not yet proven aligned (only constant multiples / `k*N` at the cheap tier; the SMT `%`
  lowering abstracts `(var+var)%const`). Useful for end-of-range computations.

### Net
The sound core (return-position constructors + var-decl-by-value + the cheap/SMT tiers) is
landed and genuinely useful. Closing gap (1) would let the emulator's address-space API state
page-alignment as an enforced precondition — directly retiring the E6 operational guard.

## UPDATE — gaps (1) and (2) RESOLVED (compiler)
The discharge sites now resolve a `type` alias to its underlying refinement predicates
(`paramRefinementTypeExpr`) at parameter and return positions, and skip the broken synthetic
`Pred(value)` runtime-check for builtin modular laws. Three independent agent attempts
converged on the identical fix; merged to `work`. Verified:
- alias-typed param **rejects** an unaligned argument under `-strict` (was vacuous);
- **alignment-mismatch is sound**: a value typed `Aligned[2048]` does NOT satisfy an
  `Aligned[4096]` parameter (rejected);
- a refinement-returning call (`page_align_down(raw)`) **entails** the param refinement and
  passes with no runtime check;
- inline-`is` parameters no longer emit the spurious `undefined identifier "Aligned"`.

Dogfooded in `elisa_tests/page_precondition_tests.elisa`: a typed fast-path
`map_page_aligned(addr: PageAddr, size: PageAddr, …)` over the real `AddressSpace_Map` — an
aligned caller compiles with no runtime guard, a misaligned caller is a COMPILE error. The
guest-facing `AddressSpace_Map` keeps its runtime guard (the guest may request any address).
This delivers the E6 design-note capability: alignment as a discharged precondition.

Gap (3) (var-decl inline-`is` parse; refined-return fact into an untyped local) remains
deferred (parser-level).
