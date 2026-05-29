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
