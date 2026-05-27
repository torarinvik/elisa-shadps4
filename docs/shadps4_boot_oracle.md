# shadPS4 Boot Oracle

The fastest path for real-game bring-up is to run shadPS4 as an executable oracle, not just read it as reference code. Instrument shadPS4 once, emit the same boot facts Elisa emits, then diff the first divergence.

## Trace Format

Each line is intentionally plain text:

```text
BOOT_ORACLE module index=0 name=eboot.bin base=0x800000000 size=0x2e04000 exec_base=0x800000000 exec_size=0x...
BOOT_ORACLE entry addr=0x800002030 argc=1 argv0=0x... rsp=0x... rdi=0x... rsi=0x... rdx=0x... rcx=0x...
BOOT_ORACLE hle seq=1 module=libkernel symbol=posix_pthread_mutex_init nid=<nid> return=0x8008c3311 rdi=0x... rsi=0x... rdx=0x... rcx=0x... r8=0x... r9=0x...
```

Keep values numeric where possible. Use hex for guest addresses.

## shadPS4 Insertion Points

These are the minimum useful hooks:

- `src/core/linker.cpp`, `Linker::LoadModule` after `module->LoadModuleToMemory(...)`: emit `BOOT_ORACLE module`.
- `src/core/linker.cpp`, `Linker::Execute` immediately before `RunMainEntry(&params)`: emit `BOOT_ORACLE entry`.
- HLE dispatch / wrapper boundary: emit `BOOT_ORACLE hle` with the guest return address and argument registers before entering the native HLE body.

The first slice should capture only module table, entry state, and the first 10 HLE calls. That is enough to settle whether Elisa’s return continuation, argument registers, module layout, and first boundary match shadPS4.

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
