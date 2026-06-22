Minimal repro:
`def ReproWithin(addr: u64, len: u64, base: u64, cap: u64) -> bool { ensure result == (addr >= base and addr + len >= addr and base + cap >= base and addr + len <= base + cap); return addr >= base and addr + len >= addr and base + cap >= base and addr + len <= base + cap }`

Finding: `elisac -strict -smt -emit semantic core/address_space.elisa` does not discharge the boolean equivalence postcondition for the u64 overflow/range law.
Impact: the invariant is enforced operationally by `AddressSpace_WithinReservation` guards before map/commit, but the equality-style helper law remains a verifier limitation.
Follow-up: add a refined range/bitvector lemma form once the strict SMT lane can discharge u64 `addr + len` overflow guards.

---
## RESOLVED (compiler fix, 2026-06-22)
`ensure result == E` where the body is `return E` (E a pure ident/field read) now proves by
reflexivity before the SMT lane (which failed it because two loads of a reference field are distinct
opaque terms). This cleared all 6 AddressSpace accessor postconditions and the memory-06 boolean
repro. Sound: a mismatched field (`ensure result == s.b; return s.a`) is still rejected. Regression
test: compiler/src/semantic/ensure_return_reflexivity_test.go.
NOTE: the broader u64 boolean-equivalence over compound overflow guards is subsumed by this for the
return-reflexive case; non-reflexive compound forms remain SMT-modeled.
