# Verbatim-Port Gap Catalog

Driving the shadPS4 → Elisa port by **transliteration**: paste a C++ file, convert
top-to-bottom with the dictionary, and let the compiler surface every missing
primitive. Each compile error that isn't a syntax mapping is a gap recorded here.

## Transliteration dictionary
| C++ | Elisa |
|---|---|
| `std::string` / `string_view` / `const char*` | `dstr` / `sview` / `cstr` |
| `std::vector<T>` (+ `for(auto&x:v)`) | `darray[T]` (+ `for x in v`) |
| `std::optional<T>` | `T?` |
| `std::unique_ptr<T>` | region-owned / linear `owned T&` |
| `std::map`/`set` | dict |
| `nullptr` / `.size()` | `null` / `.len`/`.count` |
| `Singleton<T>::Instance()` | `X_Instance()` |
| `std::jthread`/`mutex` | `ctx_thread_*` / sync runtime |

## Gaps (to implement in Elisa-core stdlib or emulator `common/`)

### Small / cheap (implement inline now, promote to common/ later)
- [x] **hex formatting** `fmt::format("{:016x}", n)` → `port_fmt_hex16` (local, in seed)
- [x] **decimal parse** `from_chars(..., 10)` → `port_parse_int` (verified)
- [x] **time parse/format** → `port_parse_hms` / `port_fmt_hms` (`{:d}:{:02d}:{:02d}`, verified)
- [ ] **general `fmt::format`** with `{}` substitution — needs a real formatter

### COMPILER gaps (Elisa-core) surfaced by the port
- ❗ **Generic `T` inference does not flow through auto-ref.** `f(x)` where
  `f[T](out: mutable T&)` and `x` is a value that needs auto-ref fails to bind
  `T` → `error: missing specialization binding for type parameter T`. Worked
  around with explicit `[T]` at call sites (`IOFile_ReadObject[...]`,
  `IOFile_WriteSpan[u8]` in `core/loader/elf.elisa`). **Proper fix:** make
  `collectTypeBindings` peel the auto-ref when matching arg→`mutable T&`.
  (This blocked the Elf/IOFile path and the kernel-memory-parity target.)
- ⚠️ **darray-append-needs-active-arena is intra-procedural** — a helper that
  pushes to a `&`-passed darray needs its own `in arena:` scope (pass the arena).
  This is exactly the allocator-threading the region-aware-container design note
  is meant to remove.

### Algorithm helpers (currently hand-rolled loops)
- [ ] `std::all_of` / `find_if` / `sort` over a range

### Host subsystems (large — build incrementally)
- [ ] **`std::filesystem`** — path join/parent/filename/exists/is_directory/relative/
      create_directory/remove_all/directory_iterator. (Partial host helpers exist:
      `Emulator_DirName/PathEndsWith/TrimTrailingSlash`.)
- [x] **`fstream` read/write** — `port_read_file_text` / `port_write_file_text`
      via `IOFile` (verified round-trip). Notes: `ofstream`(trunc) == open with
      `FileAccessMode.Create` (`Write` maps to `r+` and needs an existing file);
      the `io_read_string` **extern returns an unusable pointer** — read via
      `IOFile_ReadSpan[u8]` into an owned buffer + explicit NUL instead.
- [ ] **`sstream` tokenizing** — whitespace split + the `serial h:m:s` line parse
      (`UpdatePlayTime` core); `port_parse_int`/`port_parse_hms` ready.
- [ ] **`MntPoints`** — `Mount` / `GetHostPath`.
- [ ] **`PSF`** — `param.sfo` parser (`Open`/`GetString`/`GetInteger`).
- [ ] **`NPBindFile`** — `Load` / `GetNpCommIds` (trophies).
- [ ] **`Frontend::WindowSDL`** — window create / `IsOpen` / `WaitEvent` / `InitTimers`.
- [ ] **`std::hash<string_view>`** — for `SafeGameIdForHostPath` fallback id.
- [ ] **process restart** — `execvp` (POSIX) for `ExecuteRestartArgs`.
- [ ] **playtime thread** — `std::jthread` + `StoppableTimedWait`.

## Already provided by the port (reuse, don't rebuild)
- `Core::Loader::Elf` → `core/loader/elf.elisa`
- `MemoryManager` / `Linker` / `GameControllers` (`core/memory`, `core/linker`, `input/controller`)
- string/path helpers in `core/emulator.elisa`
- threads/sync runtime (`ctx_thread_*`, mutex/cond)

## Findings from the emulator.cpp seed (verified)
Things the transliteration taught us (compile + runtime tested):
- ✅ **`sview` is iterable** — `for ch in value` works (no index loop needed).
  `for(char c : string_view)` transliterates directly.
- ⚠️ **String building requires an arena** — `darray.push` errors without an
  active `in <arena>:` scope. Any function returning a built string must own/
  thread an arena (seed uses a module-global `port_arena`, mirroring
  `core/emulator.elisa`'s `emulator_arena`).
- ⚠️ **No `dstr.append`** — concatenate via a `push` loop or a `concat` helper.
- ⚠️ **`clone[dstr](x)`** needs an active arena scope AND `x` to be a view
  (`sview`/`dview`), not a `static u8&`.
- ❗ **`static u8&` ("C-string") `==` literal is a POINTER compare**, not content.
  Only `sview`/`cstr`/`dstr` content-compare. Use `streq(a,b)==1` for C-strings,
  **or — recommended — return `sview`/`dstr` instead of `static u8&`** so `==`
  behaves like C++ `std::string`/`string_view`.
- ⚠️ **`static u8&` → `sview` is not a postfix cast** (the `__cast__` hook covers
  `u8&?`, not `static u8&`); build via the `sview(ptr,0,-1)` constructor.

## Status
- ✅ Seed: `verbatim_port/emulator.elisa` — `IsSafeHostPathComponent`,
  `SafeGameIdForHostPath`, `port_fmt_hex16`, `port_hash_sview`, `port_concat2`,
  `port_sview_to_cstr` transliterated, compiling, and runtime-verified.
- ✅ `ReadCompiledSdkVersion` — transliterated against the real `Elf` loader
  (`core/loader/elf.elisa`), compiling. No new gaps: used auto-ref
  (`Elf_Open(elf, file)`), `for ph in elf.program_headers` (darray iteration
  replaces `find_if`), `size_of`, and a `trusted Unsafe.PointerCast` segment
  load into a locally-declared guest-layout `OrbisProcParam`. Note: `elf.elisa`
  needs `static_strlen` in scope — include `common/streq.elisa` before it.
- ⬜ `UpdatePlayTime` — next (exercises the fstream/sstream + decimal-parse gaps).
- ⬜ `Run()` / `Restart()` / `ExecuteRestartArgs()` — pend the host subsystems above.
