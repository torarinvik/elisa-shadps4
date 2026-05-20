# Requested Scope Parity Matrix

## Summary

- Total entries: `94`
- MATCHED: `94`
- MISMATCH_BEHAVIOR: `0`
- MISSING_IN_ELISA: `0`
- UNTESTED_BEHAVIOR: `0`

## Entries

| Symbol | Status | C++ | Elisa | Tests |
|---|---|---|---|---|
| `core/libraries/libs.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/ajm/ajm_instance_statistics.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/avplayer/avplayer_common.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/avplayer/avplayer_file_streamer.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `sceErrorDialogClose` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceErrorDialogGetStatus` | `MATCHED` | `STATEFUL` | `STATEFUL` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceErrorDialogInitialize` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceErrorDialogOpen` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceErrorDialogOpenDetail` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | - |
| `sceErrorDialogOpenWithReport` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | - |
| `sceErrorDialogTerminate` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceErrorDialogUpdateStatus` | `MATCHED` | `STATEFUL` | `STATEFUL` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogAbort` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogForTestFunction` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogForceClose` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetCurrentStarState` | `MATCHED` | `STATEFUL` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetPanelPositionAndForm` | `MATCHED` | `STATEFUL` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetPanelSize` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetPanelSizeExtended` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetResult` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogGetStatus` | `MATCHED` | `STATEFUL` | `STATEFUL` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogInit` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogInitInternal` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogInitInternal2` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogInitInternal3` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogSetPanelPosition` | `MATCHED` | `STATEFUL` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `sceImeDialogTerm` | `MATCHED` | `ERROR_GUARDED` | `ERROR_GUARDED` | core_libraries_ime_dialog_parity_tests.elisa |
| `core/libraries/ime/ime_dialog_ui.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/ime/ime_kb_layout.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/ime/ime_ui.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/ime/ime_ui_shared.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/save_data/save_backup.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/save_data/save_instance.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/save_data/save_memory.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `Func_2E93C0EA6A6B67C4` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `Func_C1C236728D88E177` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `Func_E9E80C474781F115` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `Func_F3DD6199DA15ED44` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayCrashDaemon` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayGetCurrentConnectionInfo` | `MATCHED` | `STATEFUL` | `STATEFUL` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayGetCurrentConnectionInfoA` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayGetCurrentInfo` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayGetEvent` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayInitialize` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayNotifyDialogOpen` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayNotifyForceCloseForCdlg` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayNotifyOpenQuickMenu` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayResumeScreenForCdlg` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayServerLock` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayServerUnLock` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlaySetMode` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlaySetProhibition` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlaySetProhibitionModeWithAppId` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayStartStandby` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayStartStreaming` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayStopStandby` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayStopStreaming` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSharePlayTerminate` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_share_play_pure_tests.elisa |
| `sceSigninDialogClose` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogGetResult` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogGetStatus` | `MATCHED` | `OTHER` | `OTHER` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogInitialize` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogOpen` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogTerminate` | `MATCHED` | `STUBBED_ORBIS_OK` | `STUBBED_ORBIS_OK` | core_libraries_signin_dialog_pure_tests.elisa |
| `sceSigninDialogUpdateStatus` | `MATCHED` | `OTHER` | `OTHER` | core_libraries_signin_dialog_pure_tests.elisa |
| `core/libraries/sysmodule/sysmodule_internal.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `core/libraries/videoout/driver.cpp::internal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `sceVideoOutAddFlipEvent` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutAddVblankEvent` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutAdjustColor` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutClose` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutColorSettingsSetGamma` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutConfigureOutputMode_` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutDeleteFlipEvent` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutDeleteVblankEvent` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetBufferLabelAddress` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetDeviceCapabilityInfo` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetEventCount` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetEventData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetEventId` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetFlipStatus` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetResolutionStatus` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutGetVblankStatus` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutIsFlipPending` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutModeSetAny_` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutOpen` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutRegisterBuffers` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutSetBufferAttribute` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutSetFlipRate` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutSetWindowModeMargins` | `MATCHED` | `STUBBED_ORBIS_OK` | `HOST_BACKED` | - |
| `sceVideoOutSubmitChangeBufferAttribute` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutSubmitFlip` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutUnregisterBuffers` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
| `sceVideoOutWaitVblank` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | core_libraries_videoout_ffi_bridge_tests.elisa |
