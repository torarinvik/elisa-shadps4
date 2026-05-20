# ImGui Big Picture Parity Matrix

## Summary

- Total symbols: `85`
- MATCHED: `85`
- MISMATCH_BEHAVIOR: `0`
- MISSING_IN_ELISA: `0`
- UNTESTED_BEHAVIOR: `0`
- Behavior buckets covered by tests: `ERROR_GUARDED, HOST_BACKED, STATEFUL`

## Entries

| Symbol | Status | C++ | Elisa | Tests |
|---|---|---|---|---|
| `GetGameIconInfo` | `MATCHED` | `HOST_BACKED` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `ImGui_ImplSDL3_CloseGamepads` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_CreateVkSurface` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_CreateWindow` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_DestroyWindow` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_GetBackendData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetClipboardText` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetSDLWindowFromViewportID` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetViewportForWindowID` | `MATCHED` | `OTHER` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetWindowFocus` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetWindowMinimized` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetWindowPos` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_GetWindowSize` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_Init` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForD3D` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForMetal` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForOpenGL` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForOther` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForSDLGPU` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitForSDLRenderer` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_InitForVulkan` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_InitMultiViewportSupport` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_KeyEventToImGuiKey` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_NewFrame` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_PlatformSetImeData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_ProcessEvent` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_RenderWindow` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_SetClipboardText` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetGamepadMode` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_SetWindowAlpha` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetWindowFocus` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetWindowPos` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetWindowSize` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetWindowTitle` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SetupPlatformHandles` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_ShowWindow` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_Shutdown` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_ShutdownMultiViewportSupport` | `MATCHED` | `OTHER` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_SwapBuffers` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_UpdateGamepadAnalog` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateGamepadButton` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateGamepads` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateKeyModifiers` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateMonitors` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDL3_UpdateMouseCursor` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateMouseData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDL3_UpdateWindow` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_CreateDeviceObjects` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_CreateFontsTexture` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDLRenderer3_DestroyDeviceObjects` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_DestroyFontsTexture` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDLRenderer3_GetBackendData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDLRenderer3_Init` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_NewFrame` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_RenderDrawData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `ImGui_ImplSDLRenderer3_SetupRenderState` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `ImGui_ImplSDLRenderer3_Shutdown` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `Launch` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `LoadSdlTextureData` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | imgui_big_picture_pure_tests.elisa, imgui_big_picture_ffi_bridge_tests.elisa |
| `LoadSdlTextureDataFromFile` | `MATCHED` | `OTHER` | `HOST_BACKED` | imgui_big_picture_ffi_bridge_tests.elisa |
| `OpenInGameSettingsDialog` | `MATCHED` | `STATEFUL` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `SDL_RenderGeometryRaw8BitColor` | `MATCHED` | `HOST_BACKED` | `HOST_BACKED` | - |
| `Saturate` | `MATCHED` | `OTHER` | `OTHER` | - |
| `SetGameIcons` | `MATCHED` | `STATEFUL` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `SettingsLayer_Draw` | `MATCHED` | `STATEFUL` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `SettingsLayer_Finish` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_AddCategory` | `MATCHED` | `HOST_BACKED` | `STATEFUL` | - |
| `SettingsWindow_AddSettingCheckbox` | `MATCHED` | `OTHER` | `OTHER` | - |
| `SettingsWindow_AddSettingCombo` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_AddSettingSliderFloat` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_AddSettingSliderInt` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_DeInit` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_DrawCategoryTabs` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_DrawGameFolderManager` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_DrawMainContent` | `MATCHED` | `STATEFUL` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `SettingsWindow_DrawProfileSelector` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_DrawSettings` | `MATCHED` | `OTHER` | `OTHER` | - |
| `SettingsWindow_DrawSettingsTable` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_GetComboIndex` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_GetProfileInfo` | `MATCHED` | `OTHER` | `STATEFUL` | - |
| `SettingsWindow_LoadSettings` | `MATCHED` | `STATEFUL` | `STATEFUL` | imgui_big_picture_pure_tests.elisa |
| `SettingsWindow_SaveInstallDirs` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_SaveSettings` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `SettingsWindow_SetupWindow` | `MATCHED` | `STATEFUL` | `STATEFUL` | - |
| `UpdateChecker` | `MATCHED` | `OTHER` | `OTHER` | - |
