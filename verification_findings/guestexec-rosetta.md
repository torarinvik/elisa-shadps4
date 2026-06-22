# Guest Exec Rosetta Verification Finding

Date: 2026-06-22
Host: arm64 macOS

## Implemented

- Added `ge_host_native_x64_runnable()` result contract: result is always `0` or `1`; cached values outside that shape are ignored and recomputed.
- Added `ge_spawn_x64_subprocess(path, argv)` boundary with contract `result >= -1`.
- Added `GuestExec_SpawnX64Subprocess(...)` public wrapper that clamps any trusted boundary result below `-1`.
- Added gate tests:
  - `emulator_guest_exec_x64_subprocess_smoke_runs_minimal_stub`
  - `emulator_guest_exec_x64_subprocess_rejects_null_path`

## Rosetta Host Probe

Manual host probe succeeded:

```text
/usr/bin/arch -x86_64 /usr/bin/true -> rc 0
/usr/bin/arch -x86_64 /usr/bin/uname -m -> x86_64, rc 0
```

This confirms the host can launch an x86_64 subprocess through Rosetta.

## Gate Result

The Elisa x64 subprocess gate did not reach runtime. It is blocked by compiler/analyzer failures before the new guest-exec tests execute.

Commands:

```text
ELISACORE_TEST_CACHE=0 elisac test emulator-guest-exec-runtime-tests-x64 --project .
ELISACORE_TEST_CACHE=0 elisac -O0 -emit test -target-triple x86_64-apple-darwin -filter x64_subprocess elisa_tests/emulator_guest_exec_runtime_tests.elisa
ELISACORE_TEST_CACHE=0 elisac -O3 -emit test -target-triple x86_64-apple-darwin -filter x64_subprocess elisa_tests/emulator_guest_exec_runtime_tests.elisa
```

Observed result: all return `rc=1`.

Initial `-O0`/project runs reached strict proof checking and failed on existing non-owned proof obligations in `core/address_space.elisa`, `core/memory.elisa`, and `core/module.elisa`, such as:

```text
core/address_space.elisa:89:5-11: ensure postcondition of "AddressSpace_PageAligned" could not be proven statically
core/memory.elisa:547:9-15: ensure postcondition of "MemoryManager_MapMemory" could not be proven statically
core/module.elisa:1576:5-11: ensure postcondition of "Module_FindByName" could not be proven statically
```

After a concurrent edit outside this task, the focused `-O0` rerun stopped earlier with a syntax error in non-owned `core/module.elisa`:

```text
core/module.elisa:1437:13-17: expected INDENT, got IDENT("self")
core/module.elisa:1438:17: unexpected token INDENT in expression
core/module.elisa:1438:24-26: expected newline, got <-
core/module.elisa:1438:24-26: unexpected token <- in expression
```

Because the gate never produced a runnable test binary, no `-O3` runtime-elision check was possible in this pass.

## Status

Rosetta subprocess availability is confirmed on the host, and the guest-exec wrapper/test lane is wired. Gate 11 remains unproven/not-run in this workspace because the compiler exits before guest-exec runtime tests execute.
