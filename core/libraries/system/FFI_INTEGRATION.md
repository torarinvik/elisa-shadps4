# System FFI Integration

Optional FFI bridges are available for runtime-backed parity:

- `userservice_ffi_bridge.elisa`
- `systemservice_ffi_bridge.elisa`

These files are not auto-included by default. This keeps pure-Elisa builds and
`elisa_tests/core_libraries_system_pure_tests.elisa` deterministic and link-safe.

## Host Symbols

If you opt in, provide these C ABI symbols in your native runtime shim:

### UserService

- `UserServiceElisa_GetInitialUser(int* out_user_id) -> int`
- `UserServiceElisa_GetLoginUserIdList(int* out_user_ids4) -> int`
- `UserServiceElisa_GetUserColor(int user_id, uint32_t* out_color) -> int`
- `UserServiceElisa_GetUserName(int user_id, char* out_name, size_t cap) -> int`
- `UserServiceElisa_GetEvent(uint32_t* out_event_kind, int* out_user_id) -> int`

### SystemService

- `SystemServiceElisa_LoadExec(char* path, void* argv) -> int`
- `SystemServiceElisa_ReceiveEvent(int* out_event_type) -> int`
- `SystemServiceElisa_GetStatusEventCount(int* out_count) -> int`
- `SystemServiceElisa_GetDisplaySafeAreaRatio(float* out_ratio) -> int`

## Wiring Pattern

1. Include the bridge file in the host-selected build surface.
2. Route runtime-sensitive calls to `*_Ffi*` wrappers.
3. Enable bridge mode explicitly when the host runtime provides validated
   bridge symbols.
4. Optional override:
   - `UserService_SetUseFfiBridge(bool)`
   - `SystemService_SetUseFfiBridge(bool)`

This pattern lets us move toward runtime-level parity without destabilizing pure
compiler/runtime tests.
