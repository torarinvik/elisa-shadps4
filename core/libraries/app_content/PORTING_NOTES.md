# app_content Elisa Port Notes

This folder now contains a full Elisa-side `app_content` module port:
- `app_content.elisa`
- `app_content_error.elisa`
- `app_content_harness.elisa`

## Runtime population hooks

The C++ implementation discovers DLC metadata (entitlement labels/keys) from emulator subsystems.
The Elisa port exposes explicit hooks so that discovery logic can populate state:

- `sceAppContentDebugReset()`
- `sceAppContentDebugSetAddcontCount(count)`
- `sceAppContentDebugSetAddcontEntry(index, label, status, key)`
- `sceAppContentDebugSetAddcontEntryLabel16(index, label16, status, key)`
- `sceAppContentLoadDiscoveredEntries(entries, entry_count)`

Recommended integration path:
1. Build discovered entry records in the caller.
2. Call `sceAppContentLoadDiscoveredEntries(...)` once.
3. Use normal API entrypoints (`sceAppContentGetAddcontInfo*`, `sceAppContentGetEntitlementKey`, `sceAppContentAddcontMount*`).

## Harness entrypoints

`app_content_harness.elisa` contains:
- `app_content_harness_run()` for direct setter-path verification
- `app_content_harness_run_bulk_loader()` for bulk-loader path verification

Both return `ORBIS_OK` on success.
