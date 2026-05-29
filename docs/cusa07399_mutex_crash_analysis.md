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
