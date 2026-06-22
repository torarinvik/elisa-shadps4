# E6 fit check — VM address refinements / region laws / frame conditions

Status: declined for this installed `elisac` build. The VM/address-space domain is a real fit, but the requested capstone cannot be expressed as a passing verification target without forcing a decorative or non-generic workaround.

Why the domain fit is genuine:

- `AddressSpace_PageAligned`, `AddressSpace_RangeNoOverflow`, and `AddressSpace_WithinReservation` are shared by real mapping/allocation flows in `core/address_space.elisa` and `core/memory.elisa`.
- A frame condition on mapping mutation would bound a real failure surface: `AddressSpace_Map` mutates only `mapped_ranges`, while the surrounding `MemoryManager` tracks sibling state in `vma_areas`, `mapped_bytes`, and direct-memory tables.

Why the requested implementation was not landed:

- `protocol Reservation: type Addr = u64 is PageAligned` is rejected by the installed compiler: associated type defaults/refinements are not accepted in protocol declarations.
- `changes self.mapped` works on concrete function signatures and rejects out-of-frame writes, but mutable protocol methods with `changes` do not dispatch from generic code. A minimal `def g[T: P](t: mutable T&)` calling a protocol method fails with `undefined identifier` / `field access requires struct type, got T`.
- The verifier does not currently use a bounded address refinement to discharge the `base + cap >= base` postcondition for a real implementation; the no-overflow law remains unproved even when `base` is refined below `MAX - cap`.

The alternative would be a concrete-only harness that demonstrates `changes` on one struct, but that would not satisfy the requested `generic map_all[M: MapTarget]` obligation and would be single-use decoration rather than a protocol/law/refinement capstone.


---

## UPDATE (2026-06-22): decline RESOLVED — E6 capstone now lands (3/3 pieces)

Two of the three original blockers were the protocol-round compiler gaps, now fixed:
- **Blocker #1** (refined associated-type DEFAULT in a protocol decl) — fixed (Gap #2).
- **Blocker #2** (`def g[T: P](t: mutable T&)` calling a protocol method failed with
  `field access requires struct type, got T`) — this was exactly the by-reference
  generic-dispatch gap fixed for E2 (Gap #4).

Implemented: `core/vm_reservation_protocol.elisa` + `elisa_tests/vm_reservation_protocol_tests.elisa`
(4/4, incl. a @property). Pieces:
1. `protocol Reservation` with a refined associated-type default `type Cap = VmCap`
   (`VmCap = u64 is VmInRange[0, 2^44]`). Refinement predicates are user **laws**
   (`law VmInRange(self,lo,hi) = self>=lo and self<=hi`), same convention as E1's
   `law InRange` — there is no built-in `InRange`/`Bounded`.
2. `protocol MapTarget: Reservation` (inheritance) with a generic `map_all[M: MapTarget]
   (m: mutable M&, ...)` that dispatches both the inherited accessor and the mutating
   `changes`-framed `record_mapping` through the `mutable M&` receiver.
3. **The no-overflow law DISCHARGES STATICALLY from the refinement** — `vm_window_end`
   / `vm_range_end` prove `addr + len >= addr` from the VmCap bound at the AFFINE tier,
   no SMT, holds under `-strict`, no runtime guard. This was blocker #3, and it lands.

### Two follow-up findings surfaced (recorded for the compiler)

- **Gap #5 (backend):** generic dispatch of a by-reference receiver (`m: mutable M&`) to
  a protocol method whose `self` is **by value** (`self: Self`) passes semantic analysis
  (the Gap #4 fix) but FAILS backend codegen: `Call parameter type does not match function
  signature` — the pointer is passed where a by-value struct load is expected. Workaround
  used here: declare the accessors with a reference receiver (`self: Self&`), which works
  end-to-end. The value-self case is the backend counterpart of the Gap #4 fix.

- **Prover-modeling gap (boolean postcondition):** the no-overflow law as an INTEGER
  postcondition (`ensure result >= addr` for `result = addr + len`) discharges at the
  affine tier. The SAME law phrased as a BOOLEAN (`def f(...) -> bool: ensure result ==
  true; return base + size >= base`) does NOT discharge even under `-smt`. The prover does
  not substitute the returned comparison expression into the `result == true` obligation
  and then discharge the underlying inequality. Prefer the integer-postcondition phrasing.

- **(design note)** Page-alignment `(addr & 0xFFF) == 0` is a MODULAR predicate, outside
  the affine/InRange refinement vocabulary, so it cannot be a `requires` (would be an
  undischargeable obligation at every call site — the E5 anti-pattern). It stays an
  operational guard inside `record_mapping`, mirroring the real `AddressSpace_Map`.

See `verification_findings/PENDING_TARGETS/E6-vm-reservation-protocol.md`.
