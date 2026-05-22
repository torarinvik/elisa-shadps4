# Elisa shadPS4 Porting Milestones

This file tracks concrete achievements in the Elisa port of the emulator. The goal is to make progress visible, preserve the story of how far the port has come, and keep future parity work anchored to verified behavior.

## 2026-05-21: Audio/Input/User Runtime Services Are Ready For First Playable Frame

- Added `emulator-runtime-services-smoke` to exercise user service initialization, pad open/read/close, and audio out/in setup under the real host backend conditions.
- The smoke reports device availability honestly: on this host, SDL audio is unavailable, OpenAL is probe-visible, and the runtime still returns deterministic no-device behavior instead of pretending hardware exists.
- Added artifact emission for runtime readiness so the parity gate can report stage-by-stage progress for user service, pad, audio output, and audio input.
- Updated the quick parity gate to run the runtime-services smoke alongside the existing boot, loader, guest-exec, and renderer checks.
- Verified commands:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
go run ./src test emulator-runtime-services-smoke --project ../elisa-shad-ps4-from-scratch
```

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
./emulator-cpp-parity --quick
```

Result: `1 test(s) selected; passed=1 skipped=0 failed=0` for the runtime smoke and `passed=14 failed=0 selected=14` for the quick gate.

## 2026-05-21: Real Game Boot Smoke Reaches Zero Unresolved Imports

- Added a real-game boot smoke test for `CUSA07399`, using the local game install at `/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/shadPS4/Games/CUSA07399`.
- The Elisa emulator path now loads the real `/app0/eboot.bin` through `Main_Run`.
- The loader mounts `/app0` and `/hostapp`, loads app-local PRX dependencies, keeps the eboot as module zero, and starts through the Elisa linker path.
- Dynamic PRX dependency loading now pulls in modules such as `/app0/sce_module/libc.prx` and `/app0/sce_module/libSceFios2.prx`.
- The strict real-game smoke assertion now requires `last_unresolved_import_count == 0`.
- Fixed the final known CUSA07399 unresolved import by wiring `libSceDiscMap` NIDs into HLE registration.
- Added a native guest-exec probe mode that enables the guest execution handoff path, records that a guest entry would be reached, and returns safely instead of jumping into arbitrary guest instructions.
- The probe recorder now distinguishes dependency/module handoffs from the main executable handoff, so the real-game smoke verifies the exact eboot start entry chosen by `Module_Start`.
- The probe mode works on unsupported hosts such as arm64 macOS by validating Elisa-side handoff bookkeeping without claiming native x86_64 guest execution is available.
- Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
go run ./src test emulator-real-game-boot-smoke --project ../elisa-shad-ps4-from-scratch
```

Result: `2 test(s) selected; passed=2 skipped=0 failed=0`.


## 2026-05-21: Loader Module-Info And PRX Symbol Lookup Hardened

- `Module_GetModuleInfoEx` now reports C++-style base-adjusted init/fini and EH-frame addresses instead of raw dynamic offsets.
- `sceKernelGetModuleInfo*`, `sceKernelGetModuleInfoForUnwind`, and module-list APIs now reflect the live Elisa linker module table for real loaded SELF/PRX modules.
- `sceKernelDlsym` now resolves real parsed PRX exports instead of always returning `ESRCH`.
- The real SELF loader test now gates module-info fields, unwind info, module list handles, and real `libc.prx` export lookup through `sceKernelDlsym`.

## 2026-05-21: Real SELF/PRX Loader Path Is Executable-Tested

- The Elisa SELF/ELF loader now parses dynamic data, import symbols, relocation records, and module metadata from real binaries.
- Shared PRX modules are loaded into memory instead of being treated as metadata-only placeholders.
- The linker can resolve imports from HLE symbols and from defined symbols exported by already-loaded guest modules.
- Guest dependency paths are mapped through `/app0/sce_module/*.prx` and loaded from the mounted host game folder when present.
- Runtime lifecycle imports such as `module_start`, `module_stop`, `compilerrt_abort_impl`, and `Need_*` are routed to safe no-op linker stubs for boot smoke purposes.
- Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
go run ./src test real-self-loader-tests --project ../elisa-shad-ps4-from-scratch
```

Result: `2 test(s) selected; passed=2 skipped=0 failed=0`.

## 2026-05-21: Core Emulator Boot Smoke Is Green

- The Elisa launcher path handles normal game launches, utility commands, invalid launches, update/patch normalization, and guest module load/start routing.
- CLI runtime flags are covered by smoke tests.
- `sceKernelLoadStartModule` style guest module routing now goes through the Elisa linker path.
- Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
go run ./src test emulator-core-boot-smoke --project ../elisa-shad-ps4-from-scratch
```

Result: `6 test(s) selected; passed=6 skipped=0 failed=0`.

## 2026-05-21: Shader Bridge Enhanced With Real Output Storage, Hash, and Diagnostic Accessors

- Added `output_hash` field (FNV-1a over SPIR-V output bytes) and `diagnostic_code` field to the bridge handle struct.
- Added `elisa_shader_recompiler_program_output_hash` C ABI accessor (returns 0 when status is Placeholder or Failed).
- Added `elisa_shader_recompiler_program_diagnostic_code` C ABI accessor (returns `REAL_LINK_NOT_AVAILABLE = 1` until the full recompiler chain is wired).
- Added `ELISA_RECOMPILER_PROGRAM_STATUS_TRANSLATED = 3` and `ELISA_RECOMPILER_PROGRAM_STATUS_FAILED_WITH_DIAGNOSTIC = 4` status constants.
- Fixed return type of status/diagnostic functions to `int64_t` so Elisa's 64-bit `int` comparison with negative values (-1 = INVALID) works correctly on ARM64 (zero-extension hazard from `int32_t` caused false failures).
- Added `shader_translate_status` and `shader_translate_output_nonzero` artifact emissions to the Elisa backend.
- Added a real GCN fragment shader fixture test (`ps0_code` from gnmdriver) verifying the bridge accepts and hashes the 8-word minimal PS4 black-frame shader correctly.
- Status is honestly `PLACEHOLDER` with `diagnostic_code = REAL_LINK_NOT_AVAILABLE` because wiring the full `TranslateProgram` path requires linking ~62 shadPS4 shader_recompiler cpp files plus sirit/boost/fmt — not yet wired into the test build.
- All 7 bridge tests pass: `7 test(s) selected; passed=7 skipped=0 failed=0`.
- Blocker: native Elisa SPIR-V emission not yet complete; full recompiler link not yet wired.
- Retirement path: replace PLACEHOLDER assertion with TRANSLATED when the Elisa build system links the full recompiler chain.

## 2026-05-21: Stub-Parity Ledger Kept Honest

- Reviewed the AJM and Companion entries that match upstream C++ stubs.
- Kept those 30 entries classified as `Stub-Parity` rather than `Native-Elisa`, because they intentionally mirror shallow C++ behavior instead of carrying deeper native functionality.
- This keeps the parity gate honest: matching a C++ stub is useful, but it is not the same thing as fully native behavior.

## Kernel Threads Porting Progress

- Ported a broad kernel threads surface into Elisa, including pthread lifecycle APIs, mutexes, condition variables, rwlocks, semaphores, event flags, cleanup handling, TLS/spec APIs, stack/TCB state helpers, and exception/signal wrapper behavior.
- Added runtime-oriented tests that spawn host-backed threads and exercise blocking, wakeup, timeout, cancellation, contention, and stress paths.
- Verified the runtime suite and differential gate against the oracle backend with real thread creation, join/detach, mutex, condvar, rwlock, semaphore, event-flag, and stress coverage.
- Added differential/parity-style tests for thread behavior so future mismatches can be caught as regressions.
- Added compiler/runtime support needed by the port, including loop `break`/`continue`, richer native callback/threading support, and improved FFI/runtime documentation.
- Current frontier: continue widening differential coverage against the C++ oracle, especially cross-platform Windows/macOS/Linux edge behavior and true guest scheduling equivalence.

## Audio Porting Progress

- Ported substantial `core/libraries/audio` behavior to Elisa.
- Added SDL/OpenAL oriented backend structure using C FFI for external C libraries.
- Added ABI decoding, batch-output validation, handle/state validation, runtime state tracking, and NID registration work for `audioout` and `audioin`.
- Added parity/runtime tests covering output, input, handle matrices, state queries, batch output, and backend behavior.
- Current frontier: keep extending backend runtime verification and compare remaining edge cases against the C++ implementation.

## AvPlayer Porting Progress

- Ported AvPlayer common/source helpers into Elisa, including source type detection and callback-backed file streamer behavior.
- Added tests for source type handling, path stripping, extension handling, file replacement, EOF reads, short reads, size reads, reset, and seek-style clamping.
- Began FFmpeg C-FFI oriented design for decode/conversion parity.
- Current frontier: finish full FFmpeg decode loops, frame conversion to guest formats, controller-thread event processing, buffer pool lifecycle, and runtime media fixtures.

## Kernel Library Porting Progress

- Ported and tested substantial parts of `core/libraries/kernel`, including coredump, aio, debug, equeue, file system, memory, process, threads, and time-oriented behavior.
- Added parity and smoke tests across many kernel surfaces.
- Continued tightening C++-matching validation order, return codes, and observable state updates.
- Current frontier: keep moving remaining C++ helper behavior into Elisa and expand real executable tests where compile/parity smoke is not enough.

## HLE Library Coverage Progress

- Added broad HLE symbol registration infrastructure through Elisa resolver tables.
- Added generated NID coverage and Aerolib NID registration for large import-surface boot compatibility.
- Added specific library registration for subsystems as they were ported, including audio, game live streaming, HMD, coredump, NP profile dialog, sign-in dialog, generated HLE symbols, Aerolib, and DiscMap.
- Current frontier: replace generic/stubbed entries with real Elisa implementations as games demand behavior beyond boot/link compatibility.

## Compiler and FFI Progress Supporting the Emulator

- Improved Elisa language/runtime support needed by the emulator port, including native callbacks, threading primitives, static OS guards, and loop control.
- Documented FFI capabilities so C libraries can be consumed directly and C++ libraries can be wrapped behind C APIs when needed.
- Removed the expectation that runtime support must stay in C when Elisa can express it; runtime pieces are being ported to Elisa where practical.
- Current frontier: keep strengthening Elisa's ability to model opaque structs, platform ABI differences, callbacks, calling conventions, and OS-specific native APIs.

## Validation Commands Worth Keeping Nearby

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/compiler"
go run ./src test emulator-real-game-boot-smoke --project ../elisa-shad-ps4-from-scratch
go run ./src test real-self-loader-tests --project ../elisa-shad-ps4-from-scratch
go run ./src test emulator-core-boot-smoke --project ../elisa-shad-ps4-from-scratch
```

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
python3 -m json.tool project.json >/tmp/elisa_project_json_check.out
clang -Wall -Wextra -Werror -c native/guest_exec_runtime.c -o /tmp/guest_exec_runtime.o
```

## Current North Star

The port has moved from isolated library parity tests to a real game boot smoke using real game files and real PRX dependency loading. The next big leap is turning boot/link success into deeper runtime execution: guest code execution, graphics/audio presentation, system-service behavior under real games, and differential parity gates that keep Elisa behavior aligned with the C++ reference while replacing C++ pieces subsystem by subsystem.

## 2026-05-21: Top-Level Emulator C++ Parity Gate Added

- Added `emulator-cpp-parity` as the single top-level validation entrypoint for emulator parity progress.
- The quick gate now runs ledger checks, ABI checks, bridge syntax checks, all current parity matrix generators/checkers, native C warning gates, real SELF loader tests, core emulator boot smoke, and CUSA07399 real-game boot smoke.
- Added `parity_workqueue.py` so project-owned C++ source/header units are classified into native Elisa, temporary bridge-adjacent, or missing workqueue buckets.
- The first quick-gate run passed: `passed=23 failed=0 selected=23`.
- Current measured workqueue snapshot: `Native-Elisa=416`, `Temporary-Cpp-Bridge=107`, `Missing=12`.
- CUSA07399 staged gate status inside the parity gate: `load/link/handoff` passes; `execute-fixture`, `execute-game`, and `frame` are still explicit next-frontier gates.

Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
./emulator-cpp-parity --quick
```

## 2026-05-21: Parity Workqueue Closed To Zero Missing Units

- Added `emulator-guest-exec-runtime-tests` with a returning synthetic native guest-exec fixture, promoting the staged gate from handoff-only to `execute-fixture` coverage.
- Added native Elisa `core/platform.elisa` IRQ lifecycle modeling and `core-platform-tests`.
- Added native Elisa `video_core/multi_level_page_table.elisa` plus page-table behavior tests.
- Added native Elisa Vulkan diagnostic policy helpers for GPU command rings and wait-timeout retry/fatal behavior.
- Tightened `parity_workqueue.py` classification so project-owned C++ source/header units now report no `Missing` bucket.
- Latest quick parity gate passed: `passed=27 failed=0 selected=27`.
- Current measured workqueue snapshot: `Native-Elisa=420`, `Temporary-Cpp-Bridge=113`, `Not-Applicable-External=2`, `Missing=0`.

Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
./emulator-cpp-parity --quick
```

## 2026-05-21: Guest-Exec Crash Guard Added

- Added guarded synthetic guest execution with signal/fault capture in `native/guest_exec_runtime.c`.
- Elisa now exposes guest-exec last status, last signal, and last fault address through `core/guest_exec.elisa`.
- Added a deliberate synthetic crash test proving guarded execution recovers instead of killing the test process.
- CUSA07399 probe now classifies the real game execution stage as armed on supported x86_64 hosts or `UNSUPPORTED_HOST` on arm64 hosts.
- Latest quick parity gate passed: `passed=27 failed=0 selected=27`.
- Local staged status: `load/link/handoff=PASS`, `execute-fixture=PASS`, `execute-game=UNSUPPORTED_HOST (arm64)`, `frame=not promoted`.

Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
./emulator-cpp-parity --quick
```

## 2026-05-21: Guarded Real Game Entry Runner Added

- Added `ElisaGuestExec_RunMainEntryGuarded`, a separate guarded game-entry runner that preserves the existing noreturn handoff path.
- On x86_64 POSIX hosts, guarded game entry forks a child, enters the guest entry, enforces a timeout, and reports parent-side exit/signal/timeout status.
- On unsupported hosts, including local arm64 macOS, guarded game entry returns `-2` deterministically.
- CUSA07399 smoke now has three staged tests: load/link/handoff, probe handoff, and guarded game-entry host-status classification.
- Latest quick parity gate passed: `passed=27 failed=0 selected=27`.

Verified command:

```sh
cd "/Users/torarinvikbjarko/Documents/Coding Projects/Go projects/Elisa-core/elisa-shad-ps4-from-scratch"
./emulator-cpp-parity --quick
```

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=9 failed=3 selected=12`
- What improved: CUSA unresolved imports are zero
- Next blocker: Gate step failed: native guest_exec_runtime warnings

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=9 failed=2 selected=11`
- What improved: CUSA unresolved imports are zero
- Next blocker: Gate step failed: elisacore test emulator-core-boot-smoke

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=9 failed=2 selected=11`
- What improved: CUSA unresolved imports are zero
- Next blocker: Gate step failed: elisacore test emulator-core-boot-smoke

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=11 failed=0 selected=11`
- What improved: all 11 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=11 failed=0 selected=11`
- What improved: all 11 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=11 failed=0 selected=11`
- What improved: all 11 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=12 failed=0 selected=12`
- What improved: all 12 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: Synthetic First-Frame Graphics Ladder Verified

- Date: 2026-05-21
- Command run: `go run ./src test emulator-renderer-first-frame-smoke --project ../elisa-shad-ps4-from-scratch`
- Result: `passed=1 failed=0 selected=1`
- What improved: synthetic renderer ladder now proves open, buffer registration, flip submit, vblank completion, and first-frame candidate without touching CUSA first-frame state
- Next blocker: keep the real CUSA first-frame gate moving while preserving the synthetic smoke as a deterministic subsystem check

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=12 failed=0 selected=12`
- What improved: all 12 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: Emulator Parity Gate Run

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=15 failed=0 selected=15`
- What improved: all 15 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-21: CUSA07399 AeroLib Fallbacks Eliminated

- Date: 2026-05-21
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=15 failed=0 selected=15`
- What improved: CUSA07399 now reports `AeroLib fallback count: 0` and `unresolved count: 0` at the guarded-entry stage
- Next blocker: advance past arm64 macOS probe-only handoff toward first guest HLE boundary on an x86_64 native-exec host

## 2026-05-21: Guest Execution Runtime Hardening

- Date: 2026-05-21
- Command run: `go run ./src test emulator-guest-exec-runtime-tests --project ../elisa-shad-ps4-from-scratch`
- Result: `passed=14 failed=0 selected=14`
- What improved: guest-exec now verifies executable mapping cleanup, guarded synthetic timeout recovery, crash capture, and the guarded main-entry trampoline ABI used by real x86_64 guest handoff
- Next blocker: run the guarded CUSA07399 entry on an x86_64 native-exec host to validate the first real guest HLE/syscall boundary; arm64 macOS remains probe-only by design

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=15 failed=0 selected=15`
- What improved: all 15 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=21 failed=0 selected=21`
- What improved: all 21 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=22 failed=0 selected=22`
- What improved: all 22 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=23 failed=0 selected=23`
- What improved: all 23 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=26 failed=0 selected=26`
- What improved: all 26 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage

## 2026-05-22: Emulator Parity Gate Run

- Date: 2026-05-22
- Command run: `./emulator-cpp-parity --quick`
- Result: `passed=27 failed=0 selected=27`
- What improved: all 27 selected gate steps passed
- Next blocker: promote execute/boundary/frame stages deeper into runtime coverage
