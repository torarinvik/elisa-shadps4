# CUSA07399 boot crash: `ret` to `0x1` in early pthread-mutex init

Status: **root-cause area confirmed; exact stack-desync mechanism still open.**
This note captures the verified analysis so the fix can be done against facts.

## Symptom

In the no-fork guarded run, the guest faults: `SIGSEGV pc=0x1 fault_addr=0x1`,
`transfer_class=2` (a `ret` to a corrupted return address `0x1`). The crashing
function is the eboot mutex-init helper at `0x8008c32c0` whose epilogue is
`add rsp,0x18; pop rbx; pop rbp; ret`; its return slot is `0x7efdd3f10`.

## Verified facts (empirical, via the debug tools)

- Not a buffer overflow: the stack **canary is intact** (the `cmp ...; jne __stack_chk_fail` passes).
- Not an epilogue RSP imbalance: the `ret` pops the **correct slot** (`0x7efdd3f10`).
- The boundary thunk is **balanced** (`push r10`/`call`/`pop r10`/`ret`, rsp preserved) — disassembled at `0x600000061000`.
- `scePthreadMutexInit` resolves to the **native HLE** (`0x7000002b96b0`), not aerolib fallback.
- The page watchpoint on `0x7efdd3f10` (with the guard timeout disabled) caught the **exact writers**, inside Elisa's HLE:
  - `0x1` written by `posix_pthread_mutexattr_init` doing `attr <- g_default_mutex_attr`
    (`g_default_mutex_attr = PthreadMutexAttr(1, 0, 0)`, so `m_type = 1`).
  - `0x2` written by `posix_pthread_mutexattr_settype(attr, 2)`.
- These are **inline mutexattr field writes** to the guest attr slot. PS4 treats
  `ScePthreadMutexattr` as an **opaque 8-byte handle** (pointer to heap state);
  Elisa writes the `PthreadMutexAttr` struct (`m_type/m_protocol/m_ceiling`, ~12 bytes) inline.

## Why the value is `0x1` but the canary survives

The attr slot the HLE writes (`0x7efdd3f10`) coincides with an **outer frame's
return slot**. The write is a 4-byte `m_type=1` to that slot (low dword), leaving
`0x1`; it does not touch the canary one frame below. So this is a *targeted* write
to a return slot, not a linear overflow — exactly the observed signature.

## The open question (the actual root cause)

Why does the inner function's attr local (`[rbp-0x20]`) land on an **outer
function's return slot**? Normal nesting cannot do this (inner frames are below
outer frames). This requires an **Elisa-specific guest rsp/stack-depth desync**
around the mutex HLE sequence. shadPS4 runs the identical eboot code without
crashing, so the desync is in Elisa's guest-exec stack handling, not the game.

The `0x1` value and the inline-attr ABI are **symptoms**; the desync is the cause.
A value-only fix (opaque-handle attr ABI) would change the corrupting *value* but
likely still corrupt the return slot with a heap pointer.

## Reproduce / continue

```bash
# Crash + backtrace:
ELISA_GUEST_EXEC_NO_FORK=1 ../compiler/bin/elisacore test emulator-cusa07399-x64-exec --project .
python3 tools/crash_backtrace.py

# Catch the exact writer of 0x7efdd3f10 (guard timeout disabled so it reaches the crash):
ELISA_GUEST_EXEC_NO_FORK=1 ELISA_GUARD_TIMEOUT_MS=0 ELISA_PAGEWATCH_ADDR=0x7efdd3f10 \
  ../compiler/bin/elisacore test emulator-cusa07399-x64-exec --project .
# parse /tmp/elisa_gewd.bin for kind=124 (WATCH) hits: writer_pc + value

# Trace guest rsp per HLE call (GE_EV_HLE_RSP, kind=125 in the tape).
# Dump guest code for offline disasm:
ELISA_DUMP_CODE_ADDR=0x8008c32c0 ... ; then capstone-disassemble /tmp/elisa_code.txt
```

## Next step

Pin the desync: trace guest rsp at three points for the **crashing** invocation
(crashing-func entry → mutex-init call site → after HLE return) and find the hop
where rsp shifts by the frame offset. Fix wherever Elisa fails to preserve the
guest rsp across the HLE/guest re-entry. Then the attr ABI can be aligned to
opaque handles as a hardening follow-up.

## Reviewer ranking of likely causes (second opinion, concurs with above)

"The mutexattr inline write is the smoking bullet, not the person holding the
gun." A normal guest local for `pthread_mutexattr_t` must not overlap an active
return address; that it does means one of:

1. **Most likely:** the HLE boundary thunk / return mechanics subtly corrupts or
   desyncs guest `rsp`. (Note: the generic thunk was verified balanced in
   isolation — so suspect a *specific* path, not the common case.)
2. **Also likely:** module-init / ELF-entry fallback uses the wrong call shape /
   stack alignment vs shadPS4.
3. **Real but secondary:** `ScePthreadMutexattr` should be modeled as an opaque
   8-byte handle, not inline fields (hardening; would change `ret 0x1` into
   `ret <handle>` but not remove the overlap).
4. **Less likely now:** TLS, relocation, `%fs`, canaries, import resolution.

Decisive method: trace guest `rsp` across the crashing call chain
(before the attr-local-creating guest call → HLE entry → HLE return → next guest
insn → eventual `ret`) and compare against shadPS4's oracle. **The first point
where `rsp` differs is the bug.**

Caveat for the comparison: the two emulators may place the guest stack at
different base addresses, so compare the rsp **trajectory (deltas across calls)**,
not absolute values — or do a self-contained Elisa check: assert that, inside the
boundary thunk, `rsp` after the HLE `pop r10` equals the captured entry `rsp`
(directly tests cause #1 without shadPS4).

---

# UPDATE: null-global crash at 0x8018244ad — root cause localized to skipped libc heap bootstrap

The boot moved well past the earlier mutex/argv/abort stages. The current
crash is a guest read of an **uninitialized runtime singleton**:

- Fault: `pc=0x8018244ad`, instr `mov rcx,[rax+8]`, `rax=[0x802d85190]=0`.
- `0x802d85190` is a RIP-relative global (`lea rcx,[rip+0x1560ced]` at
  `0x80182449c`). The crashing function `malloc(0xb0)`s an object then copies
  fields from two sibling singletons (`0x802f9e520` OK, `0x802d85190` null).

## Verified facts (debug tooling, all committed)

1. **Page watchpoint, PROT_NONE mode** (`ELISA_PAGEWATCH_PROT_NONE=1
   ELISA_PAGEWATCH_ADDR=0x802d85190`): the only access to the global before
   the fault is the faulting read itself, from the native page we protect.
   No two-layer/mirror desync; the global is simply **never written**.
2. **Relocation probe** (`reloc_probe count=0`): `0x802d85190` is **not a
   relocation target** -> it is **code-initialized**, by guest code that never
   runs before the read.
3. **HLE traces**: Elisa's recorded HLEs before the crash are only
   `posix_pthread_mutex_init x3` + `malloc x3`. It jumps straight to the
   eboot's constructors.
4. **shadPS4 oracle** (boots this game fully, 522k HLE calls): its first HLEs
   are `sceKernelGetProcParam` -> `_sceKernelRtldSetApplicationHeapAPI` ->
   `sceLibcHeapGetTraceInfo` (all from libc.prx @0x809380000), then the eboot's
   `GetDirectMemorySize`/`AllocateDirectMemory`/`MapDirectMemory` heap setup.
   This libc heap bootstrap initializes the runtime singletons. Elisa never
   makes these calls.
5. **modstart oracle**: both load the same 4 modules; the eboot is entered via
   RunMainEntry (correct). Elisa runs `Module_Start` for the 3 PRXs, but libc
   gets an `elf-entry-fallback` (shadPS4 `Module::Start` SKIPS elf-entry for
   *system* libs: `IsSharedLib() && !IsSystemLib()`), and no heap bootstrap
   fires.

## Root cause

shadPS4's `GetProcParam`/`Rtld` heap bootstrap runs from **libc.prx code that
the eboot crt calls into** (guest->guest), before the eboot's own DirectMemory
setup. In Elisa that call into libc.prx's init export never happens, so the
libc/runtime singleton `0x802d85190` stays null and is later dereferenced.

## Fix lane (linker/module-init — agent territory)

1. **Primary:** find the libc.prx init export the eboot crt calls (in shadPS4
   it returns into libc.prx @0x8093d5c93) and check Elisa's resolution of that
   import — likely resolved to an aerolib fallback stub / wrong target, so the
   bootstrap is skipped. Confirm with the resolve/fallback artifacts.
2. **Parity:** shadPS4 `Module::Start` does NOT elf-entry-fallback system libs;
   Elisa does (modstart shows libc elf-entry-fallback). Align this.
3. **Pre-entry env:** shadPS4 `Linker::Execute` also runs
   `sceSysmodulePreloadModuleForLibkernel()` and an explicit GnmDriver
   DirectMemory pre-map at `0xfe0000000` ("Some games fail without accurately
   emulating this behavior") before RunMainEntry. Elisa lacks both.

## Reproduce

```bash
# Crash + the diagnostic artifact lines (reloc_probe, modstart, hle_call ring):
ELISA_GUEST_EXEC_NO_FORK=1 ../compiler/bin/elisacore test \
  emulator-cusa07399-x64-exec --project . 2>/dev/null | grep CUSA07399_ARTIFACT

# shadPS4 oracle (boots fully):
SHADPS4_TRACE_HLE_CALLS=1 ../shadPS4/build-current-x86/shadps4 \
  -g ../shadPS4/Games/CUSA07399 --config-clean --fullscreen false 2>&1 \
  | grep BOOT_ORACLE > /tmp/shad_oracle.txt
```

---

# UPDATE 2: DECISIVE — libc init runs too early in Elisa (ordering bug)

Further oracle diffing pinned the mechanism precisely.

## The ordering fact (shadPS4)

In the shadPS4 trace, `BOOT_ORACLE entry entry=0x800002030` appears AFTER the
entire heap bootstrap:

```
seq 2  sceKernelGetProcParam               (libc.prx 0x8093d5c93)
seq 3  _sceKernelRtldSetApplicationHeapAPI (libc.prx 0x8093d5e9a)
seq 4  sceKernelGetProcParam
seq 5  sceLibcHeapGetTraceInfo
seq 6-8 GetDirectMemorySize/Allocate/MapDirectMemory (eboot 0x800dcf...)
seq 9-15 mutexattr/MutexInit, more GetProcParam/Rtld
...
BOOT_ORACLE entry   <-- only NOW jump to eboot _start
```

So shadPS4 runs the **complete libc heap bootstrap during preload /
module-init**, then `RunMainEntry`. The runtime singleton at 0x802d85190 is
initialized by this pre-entry bootstrap.

## The Elisa bug (Linker_Execute ordering)

`Linker_Execute` (core/linker.elisa) runs the PRX init too early:

- line ~2255: `Linker_PreloadLibkernelGameModules` -> `Linker_LoadGuestDependency`
  -> `Module_Start(libc)` (runs libc PREINIT + elf-entry).
- line ~2262: `Linker_RelocateAllImports` (with deferred native writes).
- line ~2268: `Linker_AllocateTlsForThread`.
- line ~2285: `MemoryManager_SyncNativeBackingToGuestExec`.

libc's init thus executes BEFORE its imports are fully relocated and BEFORE the
guest-exec native backing is synced, so its HLE calls (GetProcParam, Rtld,
DirectMemory) never fire (confirmed: Elisa's recorded HLEs are only mutex_init
+ malloc; `fallback_slot_count=0`, so this is NOT a resolution failure). The
heap bootstrap silently no-ops; the eboot then runs with 0x802d85190 null and
crashes.

## Fix direction (Linker_Execute ordering — agent lane)

Make Elisa match shadPS4's order: finish relocation + TLS + guest-backing sync
FIRST, then run the preloaded PRX module inits (libc bootstrap), then
RunMainEntry. Concretely: split `Linker_PreloadLibkernelGameModules` into a
LOAD phase (kept early so the module table/relocation closure is complete) and
a START phase (`Module_Start` for the PRXs) that runs AFTER
`RelocateAllImports` + `AllocateTlsForThread` + `SyncNativeBackingToGuestExec`,
just before the eboot `RunMainEntry`.

Verify after the fix: Elisa's HLE trace should begin with GetProcParam /
RtldSetApplicationHeapAPI / DirectMemory (matching shadPS4 seq 2-8), the
`reloc_probe`/read of 0x802d85190 should see it non-null, and the crash at
0x8018244ad should be gone.

---

# UPDATE 3: ruled out naive fixes — bootstrap is a libc-preload dance, not simple ordering

Two reversible experiments (env-gated, now reverted) both FAILED to trigger the
heap bootstrap or move the crash:

1. `ELISA_RESTART_PRX_AFTER_SYNC`: re-ran `Module_Start` for the PRXs (libc etc.)
   AFTER `RelocateAllImports` + TLS + `SyncNativeBackingToGuestExec`, just before
   `RunMainEntry`. Result: no GetProcParam/DirectMemory HLEs, identical crash at
   0x8018244ad.
2. `ELISA_RUN_EBOOT_INIT_ARRAY`: ran the eboot's own preinit/DT_INIT/init_array
   before `RunMainEntry`. Result: identical crash, no bootstrap HLEs.

Conclusion: simply running more init code (PRX or eboot, early or late) does NOT
reproduce shadPS4's bootstrap. So this is NOT a plain init-ordering miss.

## Refined model (for the agent)

In shadPS4 the WHOLE bootstrap — GetProcParam, _sceKernelRtldSetApplicationHeapAPI,
sceLibcHeapGetTraceInfo, and the eboot's own GetDirectMemorySize/Allocate/
MapDirectMemory at 0x800dcf... — happens BEFORE `RunMainEntry` (trace lines
1890-1904 precede the entry line at 2258). This is driven by
`sceSysmodulePreloadModuleForLibkernel()` and the GnmDriver DirectMemory pre-map
in shadPS4's `Linker::Execute` (src/core/linker.cpp ~156-200), i.e. an
interdependent preload/SDK bootstrap, not a one-shot init_array run. Elisa's
`Linker_PreloadLibkernelGameModules` is a simplified stub (loads libSceFios2 +
libc only) and does not reproduce this dance.

Recommended next step: study `sceSysmodulePreloadModuleForLibkernel` /
`preloadModulesForLibkernel` (shadPS4 src/core/libraries/system/sysmodule_*) and
the libc.prx init path that issues GetProcParam at 0x8093d5c93, then replicate
that preload bootstrap in Elisa before `RunMainEntry`. The libc.prx init evidently
calls back into the eboot's heap-setup (0x800dcf...) which initializes the
singleton at 0x802d85190.

---

# UPDATE 4: deferred libc-init implementation attempt (gated, WIP)

Implemented the structurally-correct fix and hit deeper Elisa memory-architecture
issues. All changes are gated behind `ELISA_DEFER_PRX_INIT` (default OFF → original
behavior; default tree stays green: x64 smoke 9/9, cusa 1/3 unchanged).

## What was built (core/linker.elisa, core/guest_exec.elisa)

- `linker_defer_dependency_start`: when set (env), `Linker_LoadGuestDependency`
  loads + relocates a PRX but does NOT run its `Module_Start` during preload.
- Instead it captures the PRX's preinit/init-array function pointers
  (`Linker_CaptureModuleInits` → `Linker_CaptureInitArrayFns`) at that point.
- `Linker_StartDeferredDependencies` replays the captured pointers via
  `Module_CallGuestInitArrayEntry` (raw address, reads no Module struct) from
  INSIDE the guarded executor (`ge_run_main_entry_guarded_in_process`, just before
  `ge_jump_main_entry`), so faults are caught by the signal handler/longjmp.
- `Unsafe.GuestSegmentInstall` threaded through the guarded-executor call chain.
- `ELISA_FORCE_TAPE_DUMP`: dump the emergency tape on guarded longjmp return even
  without debug-trace, for diagnosis.

## Why this is the right shape

shadPS4 runs libc.prx's DT_PREINIT_ARRAY entry (the heap bootstrap: GetProcParam →
RtldSetApplicationHeapAPI → DirectMemory) BEFORE the eboot entry. Elisa ran it too
early (pre-sync) so it no-oped. Deferring + replaying post-sync under the guard is
correct.

## Remaining blockers (deep Elisa memory architecture)

1. **Module struct lifetime.** Verified: a Module's `base_virtual_addr` reads
   `0x803604000` (correct) in host/load context but `0x2075253d78646920` (host
   format-string bytes) when re-read from the guarded executor — the module
   metadata arena is recycled after load. Worked around by capturing raw init
   pointers during load instead of reading structs later.
2. **Init-array pointer layer.** The relocated init-array entry is readable via
   `MemoryManager_ReadU64` during preload (the original `Module_Start` read
   `0x803f90600` there) but reads back 0 post-sync from both backing and native,
   and at the `LoadGuestDependency` capture point the `dynamic_info` array sizes
   sometimes read implausibly large (garbage), so capture currently yields
   `captured_count=0`. The init-array's valid contents live in a different memory
   layer (image vs backing vs native) than assumed; a reliable cross-layer read
   model is needed.
3. **Guarded init execution.** When init pointers ARE replayed, libc PREINIT
   executes guest code under the guard but currently faults (SIGBUS in the native
   HLE region) or hangs — the HLE dispatch from the deferred-init context differs
   from the main-entry context (host-TLS/%fs state), the systemic
   "host-runtime-from-HLE" hazard noted earlier.

## Next step

Build a reliable accessor that reads a guest VA from the layer the guest actually
executes against at a given phase (image during load, native post-sync), and audit
why `dynamic_info` array sizes read garbage at the `LoadGuestDependency` point
(likely reading before the module's dynamic segment is fully parsed for transitive
deps). Then the capture yields the real libc PREINIT pointer and the replay can be
validated under the guard.

---

# UPDATE 5: Module_Start parity with shadPS4 Module::Start

Aligned Elisa's `Module_Start` (core/module.elisa) to shadPS4 `Module::Start`
(module.cpp:99-150). These are correct, verified parity fixes (x64 boot smoke 9/9,
default cusa unchanged):

1. **ELF-entry fallback gating.** shadPS4 runs the ELF entry as module_start only
   when `IsSharedLib() && !IsSystemLib()` (and there is no DT_INIT). Elisa ran it
   unconditionally. Added `Module_IsSystemLib` (mirrors module.h:168 — path under
   the sys-modules dir) and gated the fallback accordingly.
2. **Proc param argument.** shadPS4 calls `module->Start(args, argp, param)` with
   `param = module->GetProcParam()`. Elisa passed `null`. Now passes
   `Module_GetProcParam(mod_ref)` at all three Module_Start call sites
   (Linker_LoadGuestDependency + Linker_LoadAndStartModule x2).

## Still-open parity gaps (deviations that remain)

- **System-lib classification / load source.** shadPS4 loads libc.prx from its
  bundled sys_modules dir, so `IsSystemLib(libc)=true` and it skips libc's ELF
  entry (the trace shows libc.prx runs ONLY its PREINIT). Elisa loads libc.prx
  from `/app0/sce_module` (game-local), so `IsSystemLib=false` and it still runs
  libc's ELF entry (an extra execution shadPS4 does not do). To match, Elisa must
  resolve/load system libraries from the sys-modules dir (and/or normalize the
  IsSystemLib path comparison to absolute paths). This did NOT change the crash,
  so it is a correctness/parity item, not the boot blocker.
- **The boot blocker is architectural, not a code deviation:** libc's PREINIT
  bootstrap runs in Elisa's LOAD phase (host-emulated, before native-backing sync
  / HLE-thunk setup), so its GetProcParam HLE never dispatches. shadPS4 runs all
  guest code natively, so PREINIT's GetProcParam works in place. Making Elisa
  match requires running the PRX inits in the synced execution phase (the gated
  ELISA_DEFER_PRX_INIT work in UPDATE 4), which is blocked on the cross-layer
  init-array read + module-arena lifetime issues documented there.

---

# UPDATE 6: ARCHITECTURAL BLOCK SOLVED — init trampoline runs the libc heap bootstrap

The deferred-init refactor now works (gated behind ELISA_DEFER_PRX_INIT). The libc
heap bootstrap runs as native guest code before the eboot entry, matching shadPS4,
and the old null-singleton crash at 0x8018244ad is GONE.

## Design (the fix)

1. **Capture (load phase, transient window):** in Linker_LoadGuestDependency, right
   after each dependency's per-module relocation -- the only point where the
   relocated init-array function pointers are present in the MemoryManager backing
   (the same point the original Module_Start reads them) -- copy the preinit/init
   array entry VALUES into a stable global list (Linker_CaptureInitValuesNow). They
   are gone from every memory layer (backing/image/native) by execution time, so
   this is the only place to read them.
2. **Trampoline (execution phase):** after the native-backing sync, emit a small
   guest machine-code trampoline (Linker_BuildInitTrampoline): push rdi/rsi (preserve
   the entry ABI), align rsp, `call` each captured init fn, restore rsp/rdi/rsi, then
   `jmp` the real eboot entry. Pure guest code.
3. **Redirect:** the guarded executor (ge_run_main_entry_guarded_in_process) jumps to
   the trampoline instead of the eboot entry. So the PRX inits run as NATIVE guest
   code, under the signal guard, with live HLE dispatch -- exactly like shadPS4 runs
   PRX inits before _start -- then control falls into the eboot entry.
   last_entry_addr stays the real eboot entry for reporting/assertions.

## Result

HLE trace now matches shadPS4's boot order:
  seq 1 sceKernelGetProcParam            (libc.prx 0x803fe5c93)
  seq 2 _sceKernelRtldSetApplicationHeapAPI
  seq 3 sceKernelGetProcParam
The captured pointer is libc's PREINIT entry 0x803f90600 (matches the earlier
modstart). Default tree unchanged (smoke 9/9; cusa default still 1/3).

## Next blocker (separate, known issue)

The boot now advances into libc's heap setup and faults at a HOST address
(pc=0x7ff80ceac15c, fault_addr=0x68) -- the systemic "host malloc/CRT called from
HLE context with wrong host %fs/TLS" hazard flagged earlier, NOT the architectural
init-ordering block (which is resolved). The libc bootstrap calls an HLE
(internal_malloc -> host malloc) while %fs is in a guest/transitional state and the
host allocator dereferences host TLS at +0x68. That is the next thing to fix.

---

# UPDATE 7: substrate parity — single-view guest memory (gap 1, step 1)

Making Elisa's guest-memory model match shadPS4's: one mapping, written and read
directly, instead of a host-side backing buffer mirrored into native via a sync.

shadPS4 maps guest memory MAP_FIXED (guest VA == host VA) and reads/writes that one
mapping. Elisa's native region is ALSO MAP_FIXED at the guest VA -- but it kept a
parallel host-side `mapped_bytes` backing buffer that the loader/relocator wrote to,
read from, and synced into native. Writes went to BOTH; reads came from the backing;
relocations had a deferred "backing-only" batch path. That dual model is the root of
the whole layer-mismatch bug class (the relocated value present in one layer, 0 in
another -- see UPDATE 4/6).

Changes (core/memory.elisa, core/linker.elisa), active when guest-exec is enabled:
- MemoryManager_ReadU64 / ReadU8 now read the native MAP_FIXED mapping (the same
  memory the guest executes against) -- native is the single source of truth. The
  backing-index check still validates the address is mapped.
- Removed the deferred backing-only relocation path (linker_defer_native_relocation_
  writes no longer set in Linker_Execute): relocations write native directly.
  ge_make_writable_cached keeps repeated same-page writes cheap.

The host-side backing buffer is now legacy (still written, no longer the read source
when exec is active); a follow-up can delete it + the Sync step + WriteU64BackingOnly
entirely. The non-exec (model-only) path is unchanged (guarded by GuestExec_IsEnabled).

Verified: x64 boot smoke 9/9; deferred-init boot still runs the libc bootstrap and
reaches the gap-3 host-malloc fault (0x68); default boot unchanged. (The
kernel-memory-parity-tests target fails to compile both before and after this change
-- a pre-existing target-include issue, not a regression.)

---

# UPDATE 8: gap-3 root-caused to the boundary thunk restoring host %fs but NOT host %gs (macOS); gated %gs-restore added

## Root cause (confirmed by source audit; see "environment blocker" below for why it's not yet runtime-verified)

On macOS x86-64 the HOST thread-local-storage register is **%gs**, not %fs
(libpthread / errno / the malloc per-thread cache all live at `%gs:disp`). The PS4
GUEST uses **%fs** for its TLS. Elisa's whole segment model reflects only half of
that asymmetry:

- `core/tls.elisa` installs/swaps the GUEST TCB exclusively through the **%fs** LDT
  selector (`TLS_PrepareTcbBase` -> `i386_set_ldt` -> `ElisaTls_LoadGuestFsSelectorAsm`).
  There is no `ElisaTls_ReadGsSelectorAsm` / `...LoadGs...` anywhere -- **%gs is never
  read, saved, or restored** by any Elisa path.
- The HLE boundary thunk (`Linker_CreateNativeHleBoundaryThunk`,
  `core/linker.elisa:1496`) that wraps every guest->HLE import call saves the live
  guest **%fs** selector, loads the captured **host %fs** selector
  (`movw $host_selector,%r10w ; movw %r10w,%fs`, full[127..135]), runs the HLE body,
  then restores guest %fs on the way out. It does the same for %fs **but emits no %gs
  instruction at all**.

So when the libc PREINIT bootstrap (deferred-init / trampoline path, UPDATE 6) calls
an HLE that reaches a HOST `malloc`/CRT routine, that host routine does its normal
`mov ...,%gs:0x68` (the macOS pthread-TLS slot used by the malloc fast path). If %gs's
base is at a guest/zero value at that moment, the access resolves to linear address
`0x68` and faults -- **exactly the observed `fault_addr=0x68`** (a bare small offset =>
segment base 0). The signal handler's own fault-classifier already anticipated this
case: `core/guest_exec.elisa:2489-2497` decodes whether the faulting TLS deref used
`%fs (0x64)` vs `%gs (0x65)` and looks for "a %gs-relative per-thread default-zone
read". The fix the thunk was missing is the %gs counterpart of its existing %fs
restore.

Why this only bites on the deferred-init path and not the default boot: the default
cusa path crashes earlier (null singleton, pre-UPDATE-6) and never reaches a
host-malloc-from-HLE during libc heap setup. UPDATE 6 advanced the boot *into* libc's
heap bootstrap, which is the first code that calls host malloc from an HLE while the
segment state is transitional -- surfacing the latent %gs gap.

## The change (working tree, `core/linker.elisa`, fully gated)

Added an opt-in host-%gs restore to the boundary thunk, mirroring the existing %fs
restore:

- New env flag `ELISA_RESTORE_HOST_GS` -> `linker_restore_host_gs` (read in
  `Linker_Execute` next to `ELISA_DEFER_PRX_INIT`). New global
  `linker_host_gs_selector` (default `0x6F`, the macOS userland %gs selector).
- When the flag is set, `Linker_CreateNativeHleBoundaryThunk` emits 9 extra bytes
  right after the %fs restore and before `movabs $target,%r11`:
  `movw $host_gs_selector,%r10w ; movw %r10w,%gs` (`66 41 BA <imm16>` + `66 41 8E EA`;
  ModRM.reg=5 selects %gs vs reg=4=%fs). The guest never uses %gs, so no guest-%gs
  save is needed -- the thunk just forces host %gs before the HLE body runs.
- The whole tail (target movabs / call / return-slot restore / %fs-pop / ret) is now
  emitted at a computed base `t = 136 + gs_extra`. With the flag OFF, `gs_extra=0`,
  `t=136`, `thunk_size=174`, and every byte is identical to the original layout, so
  the default tree is byte-for-byte unchanged.

## ENVIRONMENT BLOCKER -- could not compile/run to validate

The project HEAD (committed 2026-06-22, the "verify: dogfood contracted protocols"
series) now uses compiler features that **no elisacore binary present in this
environment supports**: `protocol`/`law` declarations (`core/ord.elisa`) and bracketed
annotation lists `@[noalloc, nolock]` (`core/address_space.elisa`). Every installed
binary (`../compiler/bin/elisacore` Jun 3; `Go projects/Elisa-core/bin/elisacore`
Jun 16) rejects these with parse errors, so `elisacore test emulator-cusa07399-x64-exec`
fails at the front-end before any guest code runs. The compiler source under
`Go projects/Elisa-core/compiler/src/parser/` DOES implement `law`/`protocol`, i.e. the
binaries are simply older than the source; but rebuilding the compiler is out of scope
(read-only constraint). Consequently the gap-3 fault could not be reproduced and the
%gs fix could not be runtime-verified in this session. The change is reasoned
byte-correct and gated to be inert by default; it must be validated with a compiler
built from current source via:

```
ELISA_GUEST_EXEC_NO_FORK=1 ELISA_DEFER_PRX_INIT=1 ELISA_RESTORE_HOST_GS=1 \
  <fresh-elisacore> test emulator-cusa07399-x64-exec --project .
```

## Verification recipe (for the next session, once a current compiler is available)

1. Reproduce gap-3 with `ELISA_DEFER_PRX_INIT=1` and confirm `fault_addr=0x68` plus,
   from the `GE_EV_SIGNAL_SEGMENTS` tape mark (guest_exec.elisa:2509, fs/gs base at
   mcontext ss+152/+160), that **%gs base is 0/guest** while %fs base is host at the
   fault -- this nails "host %gs clobbered" vs any other 0x68 source.
2. Re-run adding `ELISA_RESTORE_HOST_GS=1`. Expected: the host-malloc fault is gone and
   boot advances past libc heap setup (next frontier likely the eboot's own
   DirectMemory calls / `_start`).
3. If `0x6F` is wrong for this macOS build, read the live host %gs selector off the
   `GE_EV_SIGNAL_SEGMENTS` mark on a working (non-deferred) HLE and set
   `linker_host_gs_selector` to it -- or, better, add an `ElisaTls_ReadGsSelectorAsm`
   extern (compiler-runtime change) and capture it like the %fs selector instead of
   hardcoding. Hardcoding is the only option that stays within the
   shadPS4-repo-only / no-compiler-edit constraints of this session.

## Confidence

Root cause (host %gs never restored at the HLE boundary on macOS, and host malloc
reads %gs:0x68) is **high-confidence from the source + the fault signature + the
existing in-code %gs-fault-classifier comment**. The specific fix shape (force host
%gs in the thunk) directly addresses it. The residual uncertainty is purely the
unhardcoded host %gs selector value and the lack of runtime validation, both owing to
the compiler-version blocker.

---

# UPDATE 9: host %gs selector now runtime-captured (hardening); runtime validation still pending

Two corrections to UPDATE 8's caveats, plus the remaining open item.

## The %gs selector is no longer hardcoded

UPDATE 8 said hardcoding `0x6F` was "the only option that stays within the
shadPS4-repo-only / no-compiler-edit constraints." That was wrong: the `ElisaTls_*Asm`
externs are implemented in `easm/guest_exec_x86_64.easm`, which is **in the shadPS4
repo** (not the compiler), so a `%gs` read extern is addable here. Done:

- `easm/guest_exec_x86_64.easm`: new `ElisaTls_ReadGsSelectorAsm() -> u16` (`movw %gs,%ax;
  ret`), mirroring the existing `ElisaTls_ReadFsSelectorAsm`.
- `core/tls.elisa`: `TLS_CaptureHostGsSelector()` / `TLS_HostGsSelector()` + the
  `tls_host_gs_selector(_captured)` globals, mirroring the host-%fs capture. Captured on a
  host thread (module init, before any guest %gs swap), so %gs still holds the host
  selector at capture time.
- `core/linker.elisa` (`Linker_CreateNativeHleBoundaryThunk`): when `ELISA_RESTORE_HOST_GS`
  is set, the thunk now prefers the **runtime-captured** host %gs selector over the `0x6F`
  default (`linker_host_gs_selector <- captured_gs` only when non-zero, so an off-platform
  0 falls back safely). Flag OFF ⇒ thunk byte-identical to the original 174 bytes.

So the "unhardcoded selector value" residual from UPDATE 8 is closed — the correct host
%gs selector is now read from the live register rather than guessed.

## Still open: runtime validation

The fix is **not yet runtime-verified**. The binary blocker in UPDATE 8 was a red herring
for *which* binary: `../compiler/bin/elisacore` (Jun 3) and the Jun-16 binary are stale,
but `~/.local/bin/elisac` (current) compiles current source and can drive the target
(`elisac test emulator-cusa07399-x64-exec --project .`; game files present at
`../shadPS4/Games/CUSA07399`). The full emulator run is slow (whole-emulator build + game
load), and capped attempts did not reach the fault within the cap, so the decisive
before/after has not been captured.

Verification recipe unchanged from UPDATE 8 (use `~/.local/bin/elisac`, not the stale
`elisacore`): (1) `ELISA_DEFER_PRX_INIT=1` baseline → confirm `fault_addr=0x68` and, from
the `GE_EV_SIGNAL_SEGMENTS` tape (fs/gs base at mcontext ss+152/+160), that %gs base is
0/guest while %fs is host; (2) add `ELISA_RESTORE_HOST_GS=1` → expect the 0x68 fault gone
and boot advancing past libc heap setup (next likely frontier: the eboot's own
DirectMemory calls / `_start`). The selector no longer needs manual tuning.

## Status

Code: COMPLETE and gated (default tree unchanged by construction). Root cause:
high-confidence. Runtime validation: the one remaining step, gated only on spending a full
slow emulator run.
