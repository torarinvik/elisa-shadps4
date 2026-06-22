# PENDING project.json target stanza — E6 VM-reservation capstone (retry, now landed)

E6 declined on three blockers; two were the protocol-round compiler gaps now fixed
(refined associated-type default = Gap #2; generic by-ref protocol dispatch = Gap #4),
and the third (the no-overflow proof) discharges at the affine tier from the refinement.

## Files added (source-only, additive — nothing existing includes these)
- `core/vm_reservation_protocol.elisa` — `law VmInRange`, refined `VmCap`, `protocol
  Reservation` (refined assoc-type default), `protocol MapTarget: Reservation`
  (inheritance + `changes`-framed mutating method), `WindowReservation` impl, generic
  `map_count`, and the statically-proven `vm_window_end` / `vm_range_end`.
- `elisa_tests/vm_reservation_protocol_tests.elisa` — 3 `@test` + 1 `@property`.

## Verified standalone
`ELISACORE_TEST_CACHE=0 elisac -emit test elisa_tests/vm_reservation_protocol_tests.elisa`
=> `passed=4 skipped=0 failed=0`.
`ELISACORE_TEST_CACHE=0 elisac -emit semantic -strict elisa_tests/vm_reservation_protocol_tests.elisa`
=> no undischarged obligations (the no-overflow postconditions prove under -strict, no SMT).

## Proposed test-target stanza (project.json `targets`, when wired)
```json
"vm-reservation-protocol-tests": {
  "entry": "elisa_tests/vm_reservation_protocol_tests.elisa",
  "emit": "test",
  "foreign": []
}
```

## Two compiler follow-ups this surfaced (see E6-fit.md for detail)
1. Gap #5 (backend): generic by-ref receiver -> by-VALUE-self protocol method passes
   semantics but fails LLVM codegen (`Call parameter type does not match function
   signature`). Workaround: `self: Self&` accessors. The backend counterpart of Gap #4.
2. Prover-modeling gap: a no-overflow law as a BOOLEAN postcondition (`ensure result ==
   true` over `base+size >= base`) does not discharge even under -smt, while the INTEGER
   form (`ensure result >= base`) discharges at the affine tier. Prefer integer phrasing.

   RESOLVED (compiler): `normalizeBoolLiteralEnsure` in the verification engine now folds a
   literal-comparison postcondition to the bare predicate it means — `result == true` /
   `result != false` -> `result`, `result == false` / `result != true` -> `not result` —
   before discharge, routing it through the same lane that already proved bare `ensure
   result`. The capstone now carries the natural boolean form `vm_range_no_overflow`
   (`ensure result == true` over `return base + size >= base`), which discharges under -smt
   from the VmCap refinements (verified: `-emit semantic -strict -smt` EXIT=0, 0 undischarged).
   Soundness preserved: an unsatisfiable literal postcondition still declines with a
   counterexample (the rewrite is a pure logical identity, asserting nothing extra).
