# IPC C++ -> Elisa Parity Matrix

Source baseline: `core/ipc/ipc.cpp` + `core/ipc/ipc.h`
Scope: `core/ipc` only (`ipc.elisa`, `ipc_runtime.elisa`, `ipc_hooks.elisa`, `core_ipc_pure_tests.elisa`)

## State Fields and Defaults

| Area | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| IPC enabled flag | `enabled{false}` | `IpcState.enabled=false` | matched | Same default and env-gated init behavior. |
| Run semaphore | `binary_semaphore{0}` | `IpcBinarySemaphore_new(0)` | matched | Binary 0/1 behavior mirrored via atomics CAS. |
| Start semaphore | `binary_semaphore{0}` | `IpcBinarySemaphore_new(0)` | matched | Same startup lock semantics. |
| Input thread handle | `std::jthread input_thread{}` | detached raw thread in runtime path | partial | Lifecycle semantics are close but no joinable handle retained in state. |
| Additional wait/close flags | none explicit in C++ class | `run_ready/start_ready/run_wait_deadline_ms/should_close/close_reason` | partial | Pure-model helpers for deterministic tests; does not change runtime command behavior. |

## Command Parsing and Dispatch

| Command/Behavior | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| Empty command line | ignore/continue | ignore/continue | matched | |
| `RUN` | `run_semaphore.release()` | release + `run_ready=true` | matched | Extra flag used for pure wait assertions. |
| `START` | `start_semaphore.release()` | release + `start_ready=true` | matched | Extra flag used for pure wait assertions. |
| `PATCH_MEMORY` 9 args | parse + enqueue | parse + `IPC_HookAddMemoryPatch` | matched | Argument order and conversions preserved. |
| `PAUSE`/`RESUME` | debug state calls | hook calls | matched | Hook seam intentionally preserves side-effect boundary. |
| `STOP` | SDL quit event | `IPC_HookQuit` | matched | Event emission delegated via hook seam. |
| `TOGGLE_FULLSCREEN` | SDL event | `IPC_HookToggleFullscreen` | matched | Hook seam parity. |
| `ADJUST_VOLUME` | slider + adjust | hook slider + adjust | matched | Call order preserved. |
| `SET_FSR`/`SET_RCAS` | presenter flags | hook calls | matched | |
| `SET_RCAS_ATTENUATION` | int parse to float scale at presenter boundary | integer passed to hook | matched | Conversion responsibility remains in integration hook layer. |
| USB commands | backend conditional calls | hook calls | matched | Boundary preserved; no behavior drift in parser/dispatch. |
| `RELOAD_INPUTS` | parse config string | hook call with string | matched | |
| Unknown command | stderr `;UNKNOWN CMD: ...` | returns `;UNKNOWN CMD: ...` for caller emission | matched | Output content parity. |

## Runtime Input Loop / Threading / Semaphores

| Area | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| Input loop thread name | set thread name | `SetCurrentThreadName("IPC Read thread")` | matched | |
| Init starts input thread | yes when enabled | `IPC_RuntimeInitWithThread` does same | matched | |
| Init emits capabilities | stderr lines + flush | same via `IPC_RuntimeEmitLines` | matched | |
| Wait for RUN semaphore | `try_acquire_for(5s)` then exit(1) | `IpcBinarySemaphore_try_acquire_for(5_000_000_000)` then `IPC_RuntimeExit(1)` | matched | Timeout + close text parity. |
| Blocking start wait | `start_semaphore.acquire()` | `IPC_WaitForStartBlocking` acquire | matched | |
| Semaphore implementation | native blocking primitive | spin/sleep CAS loop | partial | Behavior parity targeted; implementation primitive differs (runtime constraint). |
| Thread lifetime/detach | `std::jthread` member (RAII stop token context) | detached raw thread | partial | Stop-token lifecycle not modeled in Elisa runtime. |

## Env / Init / Run-Wait / Exit Semantics

| Area | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| Env gate | `getenv("SHADPS4_ENABLE_IPC") == "true"` | same exact true-string check | matched | |
| Disable auto patches | done during init when enabled | same | matched | |
| Runtime exit path | `exit(1)` on failed RUN acquire | `IPC_RuntimeExit(1)` | matched | |
| Pure run-wait helpers | implicit in native wait | explicit helper API for tests | partial | Test-only model layer, no runtime behavior conflict. |

## Escaped-Line Protocol

| Area | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| Runtime escaped line handling | read until a non-escaped line, return final line buffer | same (final line wins) | matched | Updated to mirror C++ line-buffer overwrite behavior. |
| Pure frame escaped merge helper | consumes escaped-prefixed fragments | same final-line behavior | matched | Updated to mirror C++ `next_str` semantics. |

## Numeric Parsing

| Area | C++ | Elisa | Status | Notes |
|---|---|---|---|---|
| `next_u64` base handling | `std::stoull(..., base=0)` | prefix-aware parser (`0x`,`0`,`0b`) | matched | Added prefixed-base parity support in Elisa parser. |

## Current Gaps Summary

- `partial`: thread ownership/lifecycle model (jthread vs detached raw thread).
- `partial`: native blocking semaphore primitive vs CAS+sleep emulation.
- No currently identified `missing` or `divergent` rows in parser/dispatch/runtime command semantics for `core/ipc` scope.

## Closure Gate

Parity for `core/ipc` is considered closed when:
- `core_ipc_pure_tests.elisa` passes.
- IPC compiles without IPC-specific errors.
- Matrix contains no `missing`/`divergent` rows.

## Gate Result

- Date: 2026-05-19
- Test gate: `../compiler/elisacore-compiler -emit test core_ipc_pure_tests.elisa` passed (`11/11`).
- Compile status (IPC scope): no IPC-specific errors in the IPC test compilation path.
- Matrix status: no `missing` or `divergent` rows.
- Remaining `partial` rows are runtime primitive/modeling differences (`jthread` ownership and native blocking semaphore primitive) that do not change IPC command/protocol behavior parity.
