# Dogfood: ghost-state spec refinement on guest-mapping math

Ghost (verification-only) struct fields + a concrete-vs-model refinement obligation were dogfooded
on the emulator's memory math in `elisa_tests/ghost_model_tests.elisa`.

## What works (the real capability — STATIC verification)
```elisa
struct MappedWindow:
    base: u64
    size: u64
    ghost model_end: u64                          # erased at codegen, zero layout/arity impact
    invariant self.model_end == self.base + self.size

def window_end(self: MappedWindow&) -> u64:
    ensure result == self.model_end               # concrete arithmetic refines the abstract model
    return self.base + self.size
```
`elisac -emit semantic -strict -smt` discharges cleanly (0 obligations): the concrete end-address
computation is PROVEN to equal the abstract `model_end`, using the struct invariant. Erasure is
real — `model_end` adds no storage (`%MappedWindow = type { i64, i64 }`) and the constructor takes
only the two concrete fields. A false refinement (`ensure result == self.model_end + 1`) is rejected.

This is the intended use: ghost models verify that a concrete representation implements an abstract
spec, statically, at zero runtime cost.

## FINDING: ghost-referencing `ensure` clauses leak into runtime checks (`-emit test` breaks)
Compiling the same file with `-emit test` fails: `error: struct MappedWindow has no field model_end`.
The ghost feature strips ghost-referencing *struct invariants* from the runtime-checked set (so the
backend never reads an erased field), but a ghost-referencing **`ensure`/`requires` postcondition**
is NOT stripped — under a non-`-strict` build it lowers to a debug runtime check that reads the
erased `model_end`, which does not exist at runtime.

### Scope / impact
- Sound and fully working under `-strict -smt` (the proof path emits no runtime check).
- Breaks only when a function carrying a ghost-referencing contract clause is codegen'd for a
  non-strict/`-emit test` build.

### UPDATE — RESOLVED (compiler)
Ghost-field-referencing `requires`/`ensure` clauses are now proof-only: kept for static discharge,
stripped from the codegen-visible contract set after all discharge runs (`exprReferencesGhostField`
+ `stripGhostFieldContractsForRuntime`, mirroring the ghost-invariant split). `ghost_model_tests.elisa`
now runs under `-emit test` (passed=1) AND proves under `-strict -smt`. Soundness preserved: a false
ghost refinement is still rejected under `-strict`; a non-ghost `ensure` is still runtime-checked.

### Original recommended follow-up (now done)
Mirror the invariant treatment for contract clauses: a `requires`/`ensure` clause that references a
ghost field is PROOF-ONLY — keep it for static discharge, but strip it from the codegen-visible
`FuncDecl.EnsureValues`/`Requires` (post-discharge) so no runtime check is emitted. Add an
`exprReferencesGhostField` walk (the `Field.Ghost` flag already exists) and filter at function
finalization, exactly as `analyzeStructInvariants` splits `st.Invariants` (full) from
`stDecl.Invariants` (runtime). Then ghost models work under every emit mode.
