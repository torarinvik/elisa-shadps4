# shadPS4 Boot Oracle

The fastest path for real-game bring-up is to run shadPS4 as an executable oracle, not just read it as reference code. Instrument shadPS4 once, emit the same boot facts Elisa emits, then diff the first divergence.

## Trace Format

Each line is intentionally plain text:

```text
BOOT_ORACLE module index=0 module=eboot.bin base=0x800000000 size=0x2e04000
BOOT_ORACLE entry entry=0x800002030 argc=1
BOOT_ORACLE resolve module=libkernel library=libkernel nid=<nid> symbol=posix_pthread_mutex_init resolved=1 kind=native_hle
BOOT_ORACLE hle seq=1 module=libkernel symbol=posix_pthread_mutex_init nid=<nid> return=0x8008c3311 rdi=0x... rsi=0x... rdx=0x...
```

Keep values numeric where possible. Use hex for guest addresses. Lines may carry a
logger prefix (`[time] <Info> Core_Linker: BOOT_ORACLE ...`); the diff tool finds the
`BOOT_ORACLE` token anywhere in the line, so grepping shadPS4's log file works directly.

Field names should stay close to the Elisa side, but the diff tool normalizes the common
aliases (`name`/`module`, `addr`/`entry`, `symbol`/`name`, `resolved`/`resolved_addr`,
`kind`/`resolution`). Records are keyed by `index` (module), the literal `entry`, bare NID
(resolve), and `seq` (hle).

## shadPS4 Insertion Points

These are the minimum useful hooks:

- `src/core/linker.cpp`, `Linker::LoadModule` after `m_modules.emplace_back(...)`: emit `BOOT_ORACLE module` (**done** — module layout: index/base/size).
- `src/core/linker.cpp`, `Linker::Execute` immediately before `RunMainEntry(&params)`: emit `BOOT_ORACLE entry` (**done** — entry addr + argc).
- `src/core/linker.cpp`, `Linker::Relocate` import path (after `Resolve(...)`): emit `BOOT_ORACLE resolve nid=… symtype=… resolved=…` (**done** — per-import structural facts). Catches missing-HLE-coverage (resolved 1 vs 0) and import-table parsing divergences, keyed by bare NID. Resolved *addresses* are deliberately omitted (host HLE addr vs guest thunk VA aren't cross-comparable; the "function import resolved to a data symbol" case is caught instead by the port's local `expected_symbol_type` invariant).
- HLE call boundary: emit `BOOT_ORACLE hle` with the guest return address and argument registers. **Not done — phase 2**, gated on the first divergence being *after* import resolution.

### Why `hle` is phase 2

shadPS4 resolves each import straight to its native function address at relocation time
(`Linker::Relocate` → `Resolve` → the GOT slot at `linker.cpp`), and the guest then `call`s
that native function directly. There is **no central runtime chokepoint** every HLE call
passes through, so `BOOT_ORACLE hle` (per-call return address + arg registers) cannot be a
few log lines — it needs a logging trampoline that redirects each JUMP_SLOT through a
per-NID stub that records the return address and args, then tail-calls the real function.

Likewise, raw *resolved addresses* are not cross-comparable: shadPS4 resolves HLE imports to
**host** C++ addresses while the Elisa port resolves to **guest** thunk VAs. Only structural
facts (resolved-or-not, symbol type) would diff meaningfully there.

The implemented slice (module layout + entry + import-resolution structure) is
cross-comparable because both sides use the same ELF and `ModuleLoadBase`, so
base/size/entry and resolved-or-not facts must match. That catches module-layout bugs (the
fixed 32 MiB stride vs. real eboot size) and import classification bugs before runtime HLE
trampolines exist.

## Diff

After running instrumented shadPS4:

```bash
python3 tools/boot_oracle_diff.py \
  --expected /path/to/shadps4_cusa07399_boot_oracle.txt \
  --actual cusa07399_artifacts.txt
```

The script prints `FIRST_DIVERGENCE ...` and stops. That is the fact to fix. The crash site is usually downstream of this mismatch.

## Why This Exists

Recent bugs were cheaper to detect as first-divergence facts than as crash archaeology:

- fixed 32 MiB module stride vs. real eboot size would have appeared as a module-table mismatch;
- valid guest return continuations rejected by Elisa are visible as `hle.return` parity;
- wrong HLE argument provenance is visible at the first boundary, before it becomes a fault.

The rule: compare boot facts in execution order and fix the earliest mismatch.
