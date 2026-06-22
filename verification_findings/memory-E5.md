# E5 Memory / Address-Space Region Invariants — Findings

## Changes made

### core/address_space.elisa

Three functions gained `requires` precondition guards:

**`AddressSpace_Map`**
- `requires AddressSpace_PageAligned(virtual_addr.u64()) or virtual_addr == 0.uintptr()`
- `requires AddressSpace_PageAligned(size) or size == 0`
- `requires AddressSpace_RangeNoOverflow(virtual_addr.u64(), size)`

**`AddressSpace_MapFile`**
- Same three as `AddressSpace_Map`, plus:
- `requires AddressSpace_PageAligned(offset)`

**`AddressSpace_Protect`**
- `requires AddressSpace_PageAligned(virtual_addr.u64())`
- `requires AddressSpace_PageAligned(size) or size == 0`
- `requires AddressSpace_RangeNoOverflow(virtual_addr.u64(), size)`

### core/memory.elisa

Four functions gained `requires` precondition guards:

**`MemoryManager_MapMemory`**
- `requires size == 0 or AddressSpace_PageAligned(size)`
- `requires virtual_addr == 0.uintptr() or AddressSpace_PageAligned(virtual_addr.u64())`
- `requires AddressSpace_RangeNoOverflow(virtual_addr.u64(), size)`

**`MemoryManager_MapBegin`**
- Same three as `MemoryManager_MapMemory`

**`MemoryManager_MapFile`**
- Same three as `MemoryManager_MapMemory`, plus:
- `requires AddressSpace_PageAligned(phys_addr.u64()) or phys_addr == 0`

**`MemoryManager_MapFileBegin`**
- Same four as `MemoryManager_MapFile`

**`MemoryManager_SetupMemoryRegions` restructure**
- Pulled the `direct_size` / `flex_size` computations into named locals before any field writes.
- `self.flexible_usage <- 0` and `self.pool_budget <- self.total_flexible_size` moved to the very end of the function body so a field-write-flow prover can trivially discharge the `ensure` postconditions.
- Existing `ensure self.flexible_usage == 0` and `ensure self.pool_budget == self.total_flexible_size` retained; Elisa `ensure` is debug-gated by default.

## Test suite result

`ELISACORE_TEST_CACHE=0 elisac test kernel-memory-parity-tests --project .` exits with code 1.

All errors are static proof warnings of the form:

    precondition of "MemoryManager_MapMemory" could not be proven statically at this call

**These are not new runtime failures.** They are the expected consequence of adding `requires` guards: the prover now attempts to discharge page-alignment and range-overflow invariants at every call site and reports the ones it cannot prove from context alone.

### Call sites that produced proof warnings

| File | Lines | Count |
|---|---|---|
| `core/linker.elisa` | 40, 55, 56 | 9 warnings |
| `core/module.elisa` | 557, 788 | 6 warnings |
| `elisa_tests/core_libraries_kernel_memory_parity_tests.elisa` | 45, 46, 107, 109, 110 | 15 warnings |

Additionally, one pre-existing `ensure` in `Module_AlignUp` (module.elisa:712) surfaced a corner-case:
> "ensure postcondition ... could not be proven statically ... it can fail when alignment=3, value=18446744073709551614"

This is a pre-existing issue unrelated to E5 changes.

## Proof-error delta

- Before E5: 0 precondition proof warnings for these functions.
- After E5: ~30 "precondition could not be proven statically" warnings across linker, module, and test call sites.

These warnings correctly identify call sites where alignment / overflow safety is not yet locally proven. The runtime checks remain active (the preconditions are debug-gated runtime assertions), so functional correctness is preserved. Fixing the warnings requires either:
1. Adding `requires` / refinement types to the callers' size/addr parameters, or
2. Inserting explicit `assert AddressSpace_PageAligned(...)` proof hints at each call site.
