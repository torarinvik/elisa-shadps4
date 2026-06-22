# savedata-02 — E4: OrbisSaveDataParam full-field ABI completeness

**Date:** 2026-06-22
**Branch:** work (uncommitted)

## Summary

Completed the three missing `OrbisSaveDataParam` field decode paths (`detail`, `userParam`, `mtime`),
wired them through the PS4-ABI set/get-param entry points, extended the dir-name-search param fill,
and validated everything with a new `savedata-param-oracle-tests` target (6/6 green).

## What was missing before this task

| Field | Offset | Kind | Status before |
|-------|--------|------|---------------|
| `title` | 0 | CString\<128\> | Implemented (set/get + dir-search) |
| `subTitle` | 128 | CString\<128\> | Not in mount state at all (comment only) |
| `detail` | 256 | CString\<1024\> | Constant defined; no set/get/search implementation |
| `userParam` | 1280 | u32 | Implemented (set/get + dir-search) |
| `mtime` | 1288 | s64 | Comment in header only; no constant, no helper, no set/get |

This task added `detail` and `mtime` end-to-end.  `subTitle` was not in scope per task E4 (not
present in the existing `SaveMount` state model — left for a follow-up).

## Changes

### `savedata_abi.elisa`
- Added `SD_PARAM_OFF_MTIME = 1288` and `SD_PARAM_SIZE = 1296`
- Added `abi_s64_at` and `abi_store_s64` helpers
- `savedata_abi_set_param`: branches for `SD_PARAM_DETAIL` (type 3) and `SD_PARAM_MTIME` (type 5)
- `savedata_abi_get_param`: corresponding read-back branches with `got_size` reporting
- `savedata_abi_dir_name_search` param fill: adds `detail@256` and `mtime@1288` per result slot

### `save_mount.elisa`
- `SaveMount` and `SaveSearchHit`: `mtime: s64`, `detail: static u8&?`
- `savedata_flush_sfo`: persists DETAIL string + MTIME split as MTIME_LO/MTIME_HI integers
- `savedata_load_sfo`: restores both; reconstructs `s64` from lo/hi pair
- `savedata_search`: loads DETAIL/MTIME from each scanned param.sfo
- `savedata_mount`: zero-initialises new fields
- New: `savedata_set_param_detail`, `savedata_get_param_detail`, `savedata_set_param_mtime`, `savedata_get_param_mtime`

## Test results

```
ELISACORE_TEST_CACHE=0 elisac -emit test elisa_tests/savedata_param_oracle_tests.elisa
[ SUMMARY  ] 6 test(s) selected; passed=6 skipped=0 failed=0
```

Tests cover:
- `@property` round-trip (all five ABI fields encode→decode with identity)
- Golden byte-order vector for mtime (little-endian, negative -1 = 0xFF*8)
- Exact struct offset assertions for all CString fields + zero-padding
- Live mount set/get → umount/remount SFO persistence
- ABI entry-point path (`sceSaveDataSetParam`/`GetParam` type=3/5)

## Known gaps (not regressed, not in E4 scope)

- `subTitle@128` field is not stored in `SaveMount` or the param.sfo (separate task)
- mtime SFO representation uses two INTEGER fields (MTIME_LO/MTIME_HI) rather than a
  BINARY field; adequate for round-trip but deviates from real PS4 param.sfo schema
- `abi_s64_at` reads via `abi_u64_at` which already has the in-body bounds guard; the
  `requires off + 8 <= v.size` proof is discharged by the same struct-literal prover fix
  recorded in savedata-01.md (now green under `-strict`)
