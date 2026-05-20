# USBD Parity Matrix (C++ vs Elisa)

Last updated: 2026-05-20

Status legend:
- `Exact`: behavior matches current C++ path.
- `Exact (C++ stub)`: C++ is stubbed/unimplemented and Elisa mirrors that.
- `Deferred`: known mismatch not closed yet.

## Skylander

| Path | Status | Notes | Test |
|---|---|---|---|
| `A/C/J/L/M/Q/R/S/V/W` control commands | Exact | Response shapes + interrupt queue behavior mirrored. | `usbd_skylander_activation_and_interrupt_contract` |
| Interrupt-in default status packet | Exact | Emits `0x53` status packet when queue empty. | `usbd_skylander_default_interrupt_status_packet` |
| Figure load/remove + persistence | Exact | File-backed load and save on write/remove paths. | `usbd_skylander_persistence_roundtrip` |
| Move/temp/cancel hooks | Exact (C++ stub) | No-op parity at hook routing level. | N/A |

## Infinity

| Path | Status | Notes | Test |
|---|---|---|---|
| `0x80/81/83/90/92/93/95/96/A1/A2/A3/B4/B5` | Exact | Command IDs and payload contracts aligned. | `usbd_infinity_seed_challenge_and_stub_contract` |
| Added/removed queue precedence | Exact | Event queue consumed before normal query queue. | `usbd_dimensions_move_event_ordering_contract` |
| Figure load/remove + persistence | Exact | File-backed load and save on write/remove. | `usbd_skylander_persistence_roundtrip` |
| Move/temp/cancel hooks | Exact (C++ stub) | No-op parity at hook routing level. | N/A |

## Dimensions

| Path | Status | Notes | Test |
|---|---|---|---|
| `B0/B1/B3/C*/D2/D3/D4` | Exact | Wire response shape + checksum positions aligned. | `usbd_wrapper_control_and_interrupt_routing_contract` |
| `D0/E1/E5/FF` and unknown command bucket | Exact (C++ stub) | Queued zeroed response parity. | `usbd_dimensions_unimplemented_bucket_returns_zero_packet` |
| Added/removed packet (`0x56`) fields | Exact | Pad/index identity tracked and emitted. | `usbd_dimensions_move_event_ordering_contract` |
| Move/temp/cancel lifecycle | Exact | Move ordering: remove-dest(full), remove-src(silent), add-dest. | `usbd_dimensions_move_event_ordering_contract` |
| Figure load/remove/write persistence | Exact | File-backed state + id update on page-36 write. | `usbd_dimensions_temp_remove_and_cancel_contract` |

## USBD Wrapper / IPC Bridge

| Path | Status | Notes | Test |
|---|---|---|---|
| VID/PID backend routing | Exact | Skylander/Infinity/Dimensions mode select in `sceUsbdOpenDeviceWithVidPid`. | `usbd_wrapper_backend_selection_and_interrupt_actual_length` |
| `sceUsbdInterruptTransfer` endpoint routing + `actual_length` | Exact | Endpoint `0x01`/`0x81` handling parity for emulated backends. | `usbd_wrapper_backend_selection_and_interrupt_actual_length` |
| IPC figure hooks ownership | Exact | Dimensions active hooks; Infinity/Skylander move/temp/cancel no-op. | `usbd_wrapper_control_and_interrupt_routing_contract` |

## USBD Done Criteria

- Compile gates pass for:
  - `core/libraries/usbd/usbd.elisa`
  - `core/libraries/usbd/emulated/skylander.elisa`
  - `core/libraries/usbd/emulated/infinity.elisa`
  - `core/libraries/usbd/emulated/dimensions.elisa`
  - `core/ipc/ipc_hooks.elisa`
- Required test suites:
  - `core_libraries_usbd_parity_tests.elisa`
  - `core_libraries_port_pure_tests.elisa`
- Acceptance statement:
  - All rows above are `Exact` or `Exact (C++ stub)`, with direct test references.

## Deferred

- None currently tracked in-scope for USBD-only parity.
