# EASM typed-layout adoption candidates

Audit date: 2026-06-23

Scope:
- Compiler docs read first: `docs/103-easm-barrier-three-levels.md`, `docs/104-easm-as-typed-assembly.md`.
- Emulator assembly audited: `easm/guest_exec_x86_64.easm`, `easm/common_x86_64.easm`, `easm/common_aarch64.easm`.
- Routines audited: 25 exports.

Summary:
- Remaining EASM routines with an untyped/elementary carrier parameter dereferenced at nonzero constant record offsets: **0**.
- Existing typed-layout adoptions found in EASM: **7 routines**, covering **20 raw constant-offset reads**.
- Plain elementary carriers found: `ElisaGuestExec_LoadOrbisFpStateAsm(mxcsr: HostPtr[u32], fpucw: HostPtr[u16])` and `ElisaTls_AtomicExchangeU32Asm(ptr: HostPtr[u32])`. These only access `0(%reg)` as scalar loads/stores and are not record-layout candidates.
- Latent bug candidates from unmatched offsets: **none found** in the current EASM corpus.

Important follow-up:
- The EASM signatures are already typed for `GuestInitCtx` and `GuestMainParams`, but the Elisa extern declarations still pass these as `HostPtr[void]` in `core/tls.elisa` and `core/guest_exec.elisa`. That does not make the assembly accesses raw anymore, but it leaves the caller-facing type less precise than the EASM verifier model.

## Candidate table

| routine | file | accessed offsets | authoritative struct + fields/widths | safe-to-adopt (yes/no + reason) | reads-converted count | notes |
|---|---|---|---|---|---:|---|
| `ElisaTls_CallGuestInitEntryOnStackWithFsAsm` | `easm/guest_exec_x86_64.easm` | `0(%r14)` `movq`; `8(%r14)` `movq`; `16(%r14)` `movq`; `24(%r14)` `movq`; `32(%r14)` `movq`; `40(%r14)` `movq`; `48(%r14)` `movq` | Authoritative construction is in `core/tls.elisa` `TLS_CallGuestInitEntryOnStackWithPreparedSegment`: local `ctx: mutable u64[7]`; fields are `entry@0:u64`, `arg0@8:u64`, `arg1@16:u64`, `arg2@24:u64`, `stack_top@32:u64`, `guest_fs@40:u64`, `host_fs@48:u64`. All accesses are 8-byte `movq` reads. | Yes, already adopted in EASM as `layout GuestInitCtx` + `ctx: HostPtr[GuestInitCtx]`; ABI-identical to the previous `HostPtr[void]` carrier because layouts lower as host byte pointers. | 7 | Register provenance: `ctx` enters in `%rdi`, then `movq %rdi, %r14`; all displaced reads use `%r14`. Caller extern still says `HostPtr[void]` in `core/tls.elisa`. |
| `ElisaGuestExec_EnterMainEntryAsm` | `easm/guest_exec_x86_64.easm` | `272(%rdi)` `movq`; `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`, `@abi_layout(size, 280, align, 8, field, argc, 0, field, argv, 8, field, entry_addr, 272)`. Accessed ABI slots: `argc+padding@0:u64` via pushq, `argv@8:u64` via pushq, `entry_addr@272:u64` via movq. | Yes, already adopted in EASM as `layout GuestMainParams` + `params: HostPtr[GuestMainParams]`; accessed widths are the ABI slot widths used by the existing assembly. | 3 | `argc` is declared `s32` plus `padding:u32` in Elisa, but the assembly intentionally consumes the full 8-byte first argument slot. Caller extern still says `HostPtr[void]` in `core/guest_exec.elisa`. |
| `ElisaGuestExec_CallMainEntryAsm` | `easm/guest_exec_x86_64.easm` | `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`: `argc+padding@0:u64` ABI slot, `argv@8:u64`. | Yes, already adopted in EASM as `layout GuestMainParams`; parameter `%rsi` is copied to `%r10` before the reads. | 2 | Entry address comes from the separate `entry: GuestEntryPoint` parameter, so this routine does not read offset 272. Caller extern still says `HostPtr[void]`. |
| `ElisaGuestExec_JumpMainEntryAsm` | `easm/guest_exec_x86_64.easm` | `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`: `argc+padding@0:u64` ABI slot, `argv@8:u64`. | Yes, already adopted in EASM as `layout GuestMainParams`; parameter `%rsi` is copied to `%r10` before the reads. | 2 | No unmatched offsets. Caller extern still says `HostPtr[void]`. |
| `ElisaGuestExec_JumpMainEntryWithFsAsm` | `easm/guest_exec_x86_64.easm` | `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`: `argc+padding@0:u64` ABI slot, `argv@8:u64`. | Yes, already adopted in EASM as `layout GuestMainParams`; parameter `%rsi` is copied to `%r10` before the reads. | 2 | No unmatched offsets. Caller extern still says `HostPtr[void]`. |
| `ElisaGuestExec_JumpMainEntryOnStackAsm` | `easm/guest_exec_x86_64.easm` | `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`: `argc+padding@0:u64` ABI slot, `argv@8:u64`. | Yes, already adopted in EASM as `layout GuestMainParams`; parameter `%rsi` is copied to `%r10` before the reads. | 2 | No unmatched offsets. Caller extern still says `HostPtr[void]`. |
| `ElisaGuestExec_JumpMainEntryOnStackWithFsAsm` | `easm/guest_exec_x86_64.easm` | `8(%r10)` `pushq`; `0(%r10)` `pushq` | `core/guest_exec.elisa` `GuestEntryParams`: `argc+padding@0:u64` ABI slot, `argv@8:u64`. | Yes, already adopted in EASM as `layout GuestMainParams`; parameter `%rsi` is copied to `%r10` before the reads. | 2 | No unmatched offsets. Caller extern still says `HostPtr[void]`. |

## Non-candidates checked

| routine | file | pointer/carrier access | result |
|---|---|---|---|
| `ElisaGuestExec_LoadOrbisFpStateAsm` | `easm/guest_exec_x86_64.easm` | `ldmxcsr (%rdi)` from `mxcsr: HostPtr[u32]`; `fldcw (%rsi)` from `fpucw: HostPtr[u16]` | Scalar zero-offset loads; no record displacement to type as a layout. |
| `ElisaTls_AtomicExchangeU32Asm` | `easm/guest_exec_x86_64.easm` | `xchgl %eax, (%rdi)` through `ptr: HostPtr[u32]` | Scalar zero-offset atomic RMW; no record displacement to type as a layout. |
| `ElisaTls_ReadFs0Asm` | `easm/guest_exec_x86_64.easm` | `movq %fs:0, %rax` | Segment-base read, not a pointer-parameter dereference. |
| `FencedRDTSC`, `x86_mm_pause`, `x86_debug_break`, `debug_breakpoint` | `easm/common_x86_64.easm` | none | No pointer parameters or displaced memory operands. |
| `FencedRDTSC`, `arm64_asm_yield`, `arm64_debug_break`, `debug_breakpoint` | `easm/common_aarch64.easm` | none | No pointer parameters or displaced memory operands. |

## Ranking by reads converted

1. `ElisaTls_CallGuestInitEntryOnStackWithFsAsm`: 7 checked field reads.
2. `ElisaGuestExec_EnterMainEntryAsm`: 3 checked field reads.
3. `ElisaGuestExec_CallMainEntryAsm`: 2 checked field reads.
4. `ElisaGuestExec_JumpMainEntryAsm`: 2 checked field reads.
5. `ElisaGuestExec_JumpMainEntryWithFsAsm`: 2 checked field reads.
6. `ElisaGuestExec_JumpMainEntryOnStackAsm`: 2 checked field reads.
7. `ElisaGuestExec_JumpMainEntryOnStackWithFsAsm`: 2 checked field reads.

## Latent bug candidates

None found. Every constant displacement in the current EASM corpus either:
- is already checked by `GuestInitCtx` or `GuestMainParams`,
- is a scalar zero-offset access through an elementary pointer, or
- is a segment self-pointer read rather than a pointer-parameter record access.
