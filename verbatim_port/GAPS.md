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
> **Freshness pass 2026-05-31** (re-verified each item below against the current
> `elisacore`): the two open compiler gaps here are now **RESOLVED** — `init_to[T](x)`
> auto-ref binding and `push()` directly inside a `region r(...):` scope both compile
> clean. The explicit-`[T]` and arena-threading workarounds in the port can be unwound.
> One genuinely-new gap was found (region-param helper **calls** — see below).
- ✅ **Generic `T` inference through auto-ref — RESOLVED.** `f[T](out: mutable T&)`
  called as `f(x)` with `x` a value (incl. a struct value, and with leading non-`T`
  params) now binds `T` and compiles. The explicit-`[T]` workarounds at
  `IOFile_ReadObject[...]` / `IOFile_WriteSpan[u8]` in `core/loader/elf.elisa` are no
  longer needed (re-verify the exact call shapes when unwinding). If a residual shape
  still fails, the proper fix remains: `collectTypeBindings` peels the auto-ref when
  matching arg→`mutable T&`.
- ✅ **darray-append-needs-active-arena is intra-procedural** — RESOLVED by the
  region-parameterized-containers work. A helper `def f[region r](out: mutable
  darray[T] @r)` can now `out.push(x)` / dict-insert through the `@r` param with
  no `in arena:` scope of its own; the compiler threads the caller's region arena
  as a hidden Arena& param. (Region-containers priorities 1–3: darray push, dict
  insert/get_or_insert, nested-region escape, allocator interface.)
- ✅ **mutable / by-reference iteration EXISTS** (not a gap). `for(auto& x : v)`
  transliterates to `for mutable ref x in v:` (mutates elements / passes them to
  `mutable T&` params) or `for ref x in v:` (efficient read, no copy). Plain
  `for x in v:` is a by-value copy. See docs/17-iterators-and-for-in-mini-spec.md.
  Adopted in core/linker.elisa Linker_Resolve.
- ✅ **Direct `container.push()` inside a `region r(...):` scope — RESOLVED.** The
  candidate follow-up landed: `push()`/`extend()` directly inside a `region r(...):`
  scope now compiles (the grow sources the active region's arena). No need to wrap in
  `in <arena>:` or thread a region param for the common in-scope case.
- ❗ **NEW: region-param helper *calls* do not thread the arena.** A helper
  `def f[T, region r](out: mutable darray[T] @r, ...)` that does `out.push(x)` (even
  inside a `for`/`while`) **compiles as a definition**, but **calling** it from a region
  scope — `f(b, a.view())` or `b.f(a.view())`, even inside `in scratch:` — fails with
  `darray push requires an active in <arena>: scope`. So generic reusable container
  helpers over `@r` (e.g. a hand-written `append`/`map`/`filter`) are blocked at the
  call site. The region-container tests only *analyze* such signatures (escape-check);
  none execute a region-param push at a call site, so this slipped through.
  **Root cause (investigated 2026-05-31, fix attempted + reverted — 3 layers, not a
  one-liner):** the ABI threading itself is correct — each region param's hidden `Arena&`
  *is* lowered and registered in the backend's `s.regions` (verified: `s.regions=[r(ptr=
  true)]`), and the push emit already tries `regionArenaOwner(region)` first. The blocker
  is that the **region NAME never reaches the backend**:
  (1) in the helper body, `s.exprType(receiver)` runs through `preferBackendLiveIdentType`,
      whose live binding type for a `darray[T] @r` param has `@r` stripped → `darrayType.
      Region==""` → `regionArenaOwner("")` bails. (Fixable with a semantic-type fallback;
      that alone clears the "push requires arena" error.)
  (2) the call site (`resolveRegionArenaArgs`) reads the *argument's* region the same
      stripped way → can't pick the arena to pass → passes a null `Arena&` → "Operand is
      null".
  (3) the deeper one: even the *semantic* type of a container **local** doesn't carry its
      region — a darray local's region lives in the analyzer's `regionRefState` flow-state,
      not on `DArrayType.Region` — so there is no region name on the type to recover at the
      call arg at all. A real fix must either propagate the region onto the container type
      through to codegen, or thread `regionRefState` into the backend. High-risk arena/ABI
      area; deserves its own focused change, not a session-side patch.
  **Workaround today (use this):** pass the arena explicitly
  (`def f[T](out, a: mutable Arena&, items): in a: …`) like `cxx_vector_push`, or use the
  builtin `push`/`extend` directly inside the region/`in <arena>:` scope.

### Algorithm helpers (now in Elisa-core collections.elisa, as `@method` on `darray[T]`)
- [x] **Predicate scans** (pure `func(T) -> bool`): `v.any_of(p)`, `v.all_of(p)`,
  `v.count_if(p)`, `v.find_if(p) -> T?`. Read-only.
- [x] **Comparator ordering** (strict-weak `func(T,T) -> bool less`): `v.sort(less)`
  (in-place O(n log n) heapsort), `v.reverse()`, `v.is_sorted(less)`,
  `v.min_element(less) -> T?`, `v.max_element(less) -> T?`.
- [x] **Binary search** over a `less`-sorted range: `v.lower_bound(target, less) -> usize`,
  `v.binary_search(target, less) -> bool`.
- [x] `fold`/`accumulate` — `da.fold(init, f)` (left fold; accumulator type inferred from
  `init`). Unblocked by a compiler fix (collectTypeBindings: a concrete arg type now
  overrides a placeholder type-param binding), which also sharpens inference for any type
  param appearing only in value positions.
- [ ] `transform`/`map` to a NEW darray — needs the arena (region); use an explicit-arena
  helper (region-param call threading is the deferred compiler gap above).
- [ ] `find`/`contains` BY VALUE — needs `==` on a generic `T`; Elisa has no generic
  equality (no operator overloading). Use `find_if`/`any_of` with an equality predicate.

### Host subsystems (large — build incrementally)
- [~] **`std::filesystem`** — split into the pure string half (DONE) and the OS half (TODO):
      - [x] **path decomposition** — Elisa-core `Fs::` module (elisacore_runtime_strings.elisa):
        `Fs::filename` / `Fs::parent` / `Fs::extension` / `Fs::stem` over a path `sview`,
        returning sub-views (allocation-free), std::filesystem::path semantics. Plus
        `sview_find_last_byte` (rfind).
      - [ ] **path join** — needs an arena (builds a new string); add an explicit-arena
        `Fs::join(a, base, leaf) -> dstr`.
      - [ ] **OS queries** — `exists` / `is_directory` / `relative` / `create_directory` /
        `remove_all` / `directory_iterator`: host FFI (stat/mkdir/opendir/readdir), belongs
        with the host layer, not the string module.
- [x] **`fstream` read/write** — `port_read_file_text` / `port_write_file_text`
      via `IOFile` (verified round-trip). Notes: `ofstream`(trunc) == open with
      `FileAccessMode.Create` (`Write` maps to `r+` and needs an existing file);
      the `io_read_string` **extern returns an unusable pointer** — read via
      `IOFile_ReadSpan[u8]` into an owned buffer + explicit NUL instead.
- [x] **`sstream` tokenizing** — `port_split_lines` (getline loop), `port_nth_token`
      (whitespace `>>`), `port_subview` (bounded sub-view). Verified.
- [ ] **`MntPoints`** — `Mount` / `GetHostPath`.
- [ ] **`PSF`** — `param.sfo` parser (`Open`/`GetString`/`GetInteger`).
- [ ] **`NPBindFile`** — `Load` / `GetNpCommIds` (trophies).
- [ ] **`Frontend::WindowSDL`** — window create / `IsOpen` / `WaitEvent` / `InitTimers`.
- [x] **`std::hash<string_view>`** — Elisa-core `hash_sview(value) -> u64` (FNV-1a 64-bit).
      The port's local `port_hash_sview` can now drop to this.
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
- ✅ **`dstr`/`darray` append-many EXISTS — RESOLVED** (was "No `dstr.append`"). Use the
  builtin `dst.extend(src)` where `src` is a darray/`dview`/array of the same element
  type (verified for `darray[i64]` and `dstr` concatenation). `s.append(t)` → `s.extend(t)`.
  Threads the arena like `push` (needs an active region/`in <arena>:` scope).
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
- ✅ `UpdatePlayTime` core — `UpdatePlayTime_Core(existing, serial, secs) -> text`
  transliterated + verified (update-existing / append-new / empty cases). Full
  method still needs the host shell: `start_time`/`now()` (Clock) and
  `GetUserPath` — wire when the `Emulator` struct lands.
- ✅ file I/O — `port_read_file_text` / `port_write_file_text` (round-trip).
- ⬜ `Run()` / `Restart()` / `ExecuteRestartArgs()` — pend the host subsystems above.
