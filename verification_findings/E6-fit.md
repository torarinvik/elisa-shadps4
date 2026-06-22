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

