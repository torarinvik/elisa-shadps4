// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int g_sdl3_initialized = 0;
static int g_sdl3_backend_kind = -1;
static int g_sdl3_gamepad_mode = 0;
static int g_sdl3_manual_gamepads_count = -1;
static int g_sdl3_init_calls = 0;
static int g_sdl3_shutdown_calls = 0;
static int g_sdl3_newframe_calls = 0;
static int g_sdl3_process_event_calls = 0;
static int g_sdl3_viewport_init_calls = 0;
static int g_sdl3_viewport_shutdown_calls = 0;
static int g_sdl3_viewport_create_calls = 0;
static int g_sdl3_viewport_destroy_calls = 0;
static int g_sdl3_viewport_show_calls = 0;
static int g_sdl3_viewport_update_calls = 0;
static int g_sdl3_viewport_render_calls = 0;
static int g_sdl3_viewport_swap_calls = 0;
static int g_sdl3_update_monitors_calls = 0;

static int g_renderer_initialized = 0;
static int g_renderer_init_calls = 0;
static int g_renderer_shutdown_calls = 0;
static int g_renderer_newframe_calls = 0;
static int g_renderer_render_calls = 0;
static int g_renderer_setup_render_state_calls = 0;
static int g_renderer_geometry_raw_calls = 0;
static int g_renderer_create_fonts_calls = 0;
static int g_renderer_destroy_fonts_calls = 0;
static int g_renderer_create_device_calls = 0;
static int g_renderer_destroy_device_calls = 0;
static int g_renderer_font_texture_live = 0;
static int g_renderer_device_objects_live = 0;

static uintptr_t g_next_texture_id = 1;
static int g_texture_from_bytes_calls = 0;
static int g_texture_from_file_calls = 0;

static int g_launch_calls = 0;
static int g_last_launch_has_executable = 0;
static int g_launch_begin_calls = 0;
static int g_launch_end_calls = 0;
static int g_launch_poll_event_calls = 0;
static int g_launch_render_present_calls = 0;
static int g_launch_should_continue_budget = 1;
static int g_launch_active = 0;
static int g_launch_window_live = 0;
static int g_launch_renderer_live = 0;
static int g_launch_imgui_context_live = 0;
static int g_launch_settings_open_count = 0;
static int g_launch_queued_events[64];
static int g_launch_event_head = 0;
static int g_launch_event_count = 0;
static int g_launch_seq = 0;
static int g_launch_seq_begin = 0;
static int g_launch_seq_init_sdl3 = 0;
static int g_launch_seq_init_renderer = 0;
static int g_launch_seq_shutdown_renderer = 0;
static int g_launch_seq_shutdown_sdl3 = 0;
static int g_launch_seq_destroy_imgui = 0;
static int g_launch_seq_end = 0;
static int g_force_window_create_fail = 0;
static int g_force_sdl3_init_fail = 0;
static int g_force_renderer_init_fail = 0;

void ImguiBigPictureBridge_Reset(void) {
    g_sdl3_initialized = 0;
    g_sdl3_backend_kind = -1;
    g_sdl3_gamepad_mode = 0;
    g_sdl3_manual_gamepads_count = -1;
    g_sdl3_init_calls = 0;
    g_sdl3_shutdown_calls = 0;
    g_sdl3_newframe_calls = 0;
    g_sdl3_process_event_calls = 0;
    g_sdl3_viewport_init_calls = 0;
    g_sdl3_viewport_shutdown_calls = 0;
    g_sdl3_viewport_create_calls = 0;
    g_sdl3_viewport_destroy_calls = 0;
    g_sdl3_viewport_show_calls = 0;
    g_sdl3_viewport_update_calls = 0;
    g_sdl3_viewport_render_calls = 0;
    g_sdl3_viewport_swap_calls = 0;
    g_sdl3_update_monitors_calls = 0;

    g_renderer_initialized = 0;
    g_renderer_init_calls = 0;
    g_renderer_shutdown_calls = 0;
    g_renderer_newframe_calls = 0;
    g_renderer_render_calls = 0;
    g_renderer_setup_render_state_calls = 0;
    g_renderer_geometry_raw_calls = 0;
    g_renderer_create_fonts_calls = 0;
    g_renderer_destroy_fonts_calls = 0;
    g_renderer_create_device_calls = 0;
    g_renderer_destroy_device_calls = 0;
    g_renderer_font_texture_live = 0;
    g_renderer_device_objects_live = 0;

    g_next_texture_id = 1;
    g_texture_from_bytes_calls = 0;
    g_texture_from_file_calls = 0;

    g_launch_calls = 0;
    g_last_launch_has_executable = 0;
    g_launch_begin_calls = 0;
    g_launch_end_calls = 0;
    g_launch_poll_event_calls = 0;
    g_launch_render_present_calls = 0;
    g_launch_should_continue_budget = 1;
    g_launch_active = 0;
    g_launch_window_live = 0;
    g_launch_renderer_live = 0;
    g_launch_imgui_context_live = 0;
    g_launch_settings_open_count = 0;
    g_launch_event_head = 0;
    g_launch_event_count = 0;
    memset(g_launch_queued_events, 0, sizeof(g_launch_queued_events));
    g_launch_seq = 0;
    g_launch_seq_begin = 0;
    g_launch_seq_init_sdl3 = 0;
    g_launch_seq_init_renderer = 0;
    g_launch_seq_shutdown_renderer = 0;
    g_launch_seq_shutdown_sdl3 = 0;
    g_launch_seq_destroy_imgui = 0;
    g_launch_seq_end = 0;
    g_force_window_create_fail = 0;
    g_force_sdl3_init_fail = 0;
    g_force_renderer_init_fail = 0;
}

int ImguiBigPictureBridge_SDL3_Init(int backend_kind, void* window, void* renderer, void* gl_context) {
    (void)renderer;
    (void)gl_context;
    g_sdl3_init_calls++;
    if (window == NULL || g_force_sdl3_init_fail) {
        return 0;
    }
    g_sdl3_backend_kind = backend_kind;
    g_sdl3_initialized = 1;
    g_launch_seq++;
    if (g_launch_seq_init_sdl3 == 0) {
        g_launch_seq_init_sdl3 = g_launch_seq;
    }
    return 1;
}

void ImguiBigPictureBridge_SDL3_Shutdown(void) {
    g_sdl3_shutdown_calls++;
    g_sdl3_initialized = 0;
    g_launch_seq++;
    if (g_launch_seq_shutdown_sdl3 == 0) {
        g_launch_seq_shutdown_sdl3 = g_launch_seq;
    }
}

void ImguiBigPictureBridge_SDL3_NewFrame(void) {
    if (g_sdl3_initialized) {
        g_sdl3_newframe_calls++;
    }
}

int ImguiBigPictureBridge_SDL3_ProcessEvent(const void* event_ptr) {
    g_sdl3_process_event_calls++;
    if (!g_sdl3_initialized) {
        return 0;
    }
    return event_ptr != NULL ? 1 : 0;
}

void ImguiBigPictureBridge_SDL3_SetGamepadMode(int mode, void* manual_gamepads_array, int manual_gamepads_count) {
    (void)manual_gamepads_array;
    g_sdl3_gamepad_mode = mode;
    g_sdl3_manual_gamepads_count = manual_gamepads_count;
}

void ImguiBigPictureBridge_SDL3_UpdateMonitors(void) {
    g_sdl3_update_monitors_calls++;
}

void ImguiBigPictureBridge_SDL3_InitMultiViewportSupport(void* window, void* gl_context) {
    (void)window;
    (void)gl_context;
    g_sdl3_viewport_init_calls++;
}

void ImguiBigPictureBridge_SDL3_ShutdownMultiViewportSupport(void) {
    g_sdl3_viewport_shutdown_calls++;
}

void ImguiBigPictureBridge_SDL3_CreateWindow(void* viewport) {
    (void)viewport;
    g_sdl3_viewport_create_calls++;
}

void ImguiBigPictureBridge_SDL3_DestroyWindow(void* viewport) {
    (void)viewport;
    g_sdl3_viewport_destroy_calls++;
}

void ImguiBigPictureBridge_SDL3_ShowWindow(void* viewport) {
    (void)viewport;
    g_sdl3_viewport_show_calls++;
}

void ImguiBigPictureBridge_SDL3_UpdateWindow(void* viewport) {
    (void)viewport;
    g_sdl3_viewport_update_calls++;
}

void ImguiBigPictureBridge_SDL3_RenderWindow(void* viewport, void* arg) {
    (void)viewport;
    (void)arg;
    g_sdl3_viewport_render_calls++;
}

void ImguiBigPictureBridge_SDL3_SwapBuffers(void* viewport, void* arg) {
    (void)viewport;
    (void)arg;
    g_sdl3_viewport_swap_calls++;
}

int ImguiBigPictureBridge_SDLRenderer3_Init(void* renderer) {
    g_renderer_init_calls++;
    if (renderer == NULL || g_force_renderer_init_fail) {
        return 0;
    }
    g_renderer_initialized = 1;
    g_launch_seq++;
    if (g_launch_seq_init_renderer == 0) {
        g_launch_seq_init_renderer = g_launch_seq;
    }
    return 1;
}

void ImguiBigPictureBridge_SDLRenderer3_Shutdown(void) {
    g_renderer_shutdown_calls++;
    g_renderer_initialized = 0;
    g_renderer_font_texture_live = 0;
    g_renderer_device_objects_live = 0;
    g_launch_seq++;
    if (g_launch_seq_shutdown_renderer == 0) {
        g_launch_seq_shutdown_renderer = g_launch_seq;
    }
}

void ImguiBigPictureBridge_SDLRenderer3_NewFrame(void) {
    if (g_renderer_initialized) {
        g_renderer_newframe_calls++;
    }
}

void ImguiBigPictureBridge_SDLRenderer3_SetupRenderState(void* renderer) {
    (void)renderer;
    g_renderer_setup_render_state_calls++;
}

int ImguiBigPictureBridge_SDLRenderer3_RenderGeometryRaw8BitColor(void* renderer,
                                                                   void* texture,
                                                                   const void* xy,
                                                                   int xy_stride,
                                                                   const void* color,
                                                                   int color_stride,
                                                                   const void* uv,
                                                                   int uv_stride,
                                                                   int num_vertices,
                                                                   const void* indices,
                                                                   int num_indices,
                                                                   int size_indices) {
    (void)renderer;
    (void)texture;
    (void)xy;
    (void)xy_stride;
    (void)color;
    (void)color_stride;
    (void)uv;
    (void)uv_stride;
    (void)num_vertices;
    (void)indices;
    (void)num_indices;
    (void)size_indices;
    g_renderer_geometry_raw_calls++;
    return 0;
}

void ImguiBigPictureBridge_SDLRenderer3_RenderDrawData(void* draw_data, void* renderer) {
    (void)draw_data;
    (void)renderer;
    if (g_renderer_initialized) {
        g_renderer_render_calls++;
    }
}

int ImguiBigPictureBridge_SDLRenderer3_CreateFontsTexture(void) {
    g_renderer_create_fonts_calls++;
    if (!g_renderer_initialized) {
        return 0;
    }
    g_renderer_font_texture_live = 1;
    return 1;
}

void ImguiBigPictureBridge_SDLRenderer3_DestroyFontsTexture(void) {
    g_renderer_destroy_fonts_calls++;
    g_renderer_font_texture_live = 0;
}

int ImguiBigPictureBridge_SDLRenderer3_CreateDeviceObjects(void) {
    g_renderer_create_device_calls++;
    if (!g_renderer_initialized) {
        return 0;
    }
    g_renderer_device_objects_live = 1;
    return 1;
}

void ImguiBigPictureBridge_SDLRenderer3_DestroyDeviceObjects(void) {
    g_renderer_destroy_device_calls++;
    g_renderer_device_objects_live = 0;
}

void* ImguiBigPictureBridge_LoadTextureFromBytes(const void* data, uint32_t size) {
    if (data == NULL || size == 0u) {
        return NULL;
    }
    g_texture_from_bytes_calls++;
    uintptr_t id = g_next_texture_id++;
    return (void*)id;
}

void* ImguiBigPictureBridge_LoadTextureFromFile(const char* path) {
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    g_texture_from_file_calls++;
    uintptr_t id = g_next_texture_id++;
    return (void*)id;
}

void ImguiBigPictureBridge_RecordLaunch(const void* executable_name) {
    g_launch_calls++;
    g_last_launch_has_executable = executable_name != NULL ? 1 : 0;
}

void ImguiBigPictureBridge_LaunchBegin(const void* executable_name) {
    g_launch_begin_calls++;
    g_launch_calls++;
    g_last_launch_has_executable = executable_name != NULL ? 1 : 0;
    g_launch_active = 1;
    g_launch_window_live = 0;
    g_launch_renderer_live = 0;
    g_launch_imgui_context_live = 0;
    g_launch_poll_event_calls = 0;
    g_launch_render_present_calls = 0;
    g_launch_should_continue_budget = 1;
    g_launch_event_head = 0;
    g_launch_event_count = 0;
    memset(g_launch_queued_events, 0, sizeof(g_launch_queued_events));
    g_launch_seq = 0;
    g_launch_seq++;
    g_launch_seq_begin = g_launch_seq;
    g_launch_seq_init_sdl3 = 0;
    g_launch_seq_init_renderer = 0;
    g_launch_seq_shutdown_renderer = 0;
    g_launch_seq_shutdown_sdl3 = 0;
    g_launch_seq_destroy_imgui = 0;
    g_launch_seq_end = 0;
}

void ImguiBigPictureBridge_LaunchSetContinueBudget(int frames) {
    g_launch_should_continue_budget = frames < 0 ? 0 : frames;
}

int ImguiBigPictureBridge_LaunchShouldContinue(void) {
    if (g_launch_should_continue_budget <= 0) {
        return 0;
    }
    g_launch_should_continue_budget--;
    return 1;
}

void ImguiBigPictureBridge_LaunchQueueEventType(int event_type) {
    if (g_launch_event_count >= 64) {
        return;
    }
    int tail = (g_launch_event_head + g_launch_event_count) % 64;
    g_launch_queued_events[tail] = event_type;
    g_launch_event_count++;
}

int ImguiBigPictureBridge_LaunchPollEventType(void) {
    g_launch_poll_event_calls++;
    if (g_launch_event_count <= 0) {
        return 0;
    }
    int event_type = g_launch_queued_events[g_launch_event_head];
    g_launch_event_head = (g_launch_event_head + 1) % 64;
    g_launch_event_count--;
    return event_type;
}

void ImguiBigPictureBridge_LaunchCreateWindowAndRenderer(void) {
    if (g_force_window_create_fail) {
        g_launch_window_live = 0;
        g_launch_renderer_live = 0;
        return;
    }
    g_launch_window_live = 1;
    g_launch_renderer_live = 1;
}

void ImguiBigPictureBridge_LaunchDestroyWindowAndRenderer(void) {
    g_launch_window_live = 0;
    g_launch_renderer_live = 0;
}

void ImguiBigPictureBridge_LaunchCreateImGuiContext(void) {
    g_launch_imgui_context_live = 1;
}

void ImguiBigPictureBridge_LaunchDestroyImGuiContext(void) {
    g_launch_imgui_context_live = 0;
    g_launch_seq++;
    if (g_launch_seq_destroy_imgui == 0) {
        g_launch_seq_destroy_imgui = g_launch_seq;
    }
}

void ImguiBigPictureBridge_LaunchRenderPresent(void) {
    g_launch_render_present_calls++;
}

void ImguiBigPictureBridge_LaunchRecordSettingsOpen(void) {
    g_launch_settings_open_count++;
}

void ImguiBigPictureBridge_LaunchEnd(void) {
    g_launch_end_calls++;
    g_launch_active = 0;
    g_launch_seq++;
    g_launch_seq_end = g_launch_seq;
}

void ImguiBigPictureBridge_SetForceWindowCreateFail(int enabled) {
    g_force_window_create_fail = enabled != 0 ? 1 : 0;
}

void ImguiBigPictureBridge_SetForceSDL3InitFail(int enabled) {
    g_force_sdl3_init_fail = enabled != 0 ? 1 : 0;
}

void ImguiBigPictureBridge_SetForceRendererInitFail(int enabled) {
    g_force_renderer_init_fail = enabled != 0 ? 1 : 0;
}

int ImguiBigPictureBridge_GetSDL3Initialized(void) { return g_sdl3_initialized; }
int ImguiBigPictureBridge_GetSDL3BackendKind(void) { return g_sdl3_backend_kind; }
int ImguiBigPictureBridge_GetSDL3GamepadMode(void) { return g_sdl3_gamepad_mode; }
int ImguiBigPictureBridge_GetSDL3ManualGamepadsCount(void) { return g_sdl3_manual_gamepads_count; }
int ImguiBigPictureBridge_GetSDL3InitCalls(void) { return g_sdl3_init_calls; }
int ImguiBigPictureBridge_GetSDL3ShutdownCalls(void) { return g_sdl3_shutdown_calls; }
int ImguiBigPictureBridge_GetSDL3NewFrameCalls(void) { return g_sdl3_newframe_calls; }
int ImguiBigPictureBridge_GetSDL3ProcessEventCalls(void) { return g_sdl3_process_event_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportInitCalls(void) { return g_sdl3_viewport_init_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportShutdownCalls(void) { return g_sdl3_viewport_shutdown_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportCreateCalls(void) { return g_sdl3_viewport_create_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportDestroyCalls(void) { return g_sdl3_viewport_destroy_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportShowCalls(void) { return g_sdl3_viewport_show_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportUpdateCalls(void) { return g_sdl3_viewport_update_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportRenderCalls(void) { return g_sdl3_viewport_render_calls; }
int ImguiBigPictureBridge_GetSDL3ViewportSwapCalls(void) { return g_sdl3_viewport_swap_calls; }
int ImguiBigPictureBridge_GetSDL3UpdateMonitorsCalls(void) { return g_sdl3_update_monitors_calls; }

int ImguiBigPictureBridge_GetRendererInitialized(void) { return g_renderer_initialized; }
int ImguiBigPictureBridge_GetRendererInitCalls(void) { return g_renderer_init_calls; }
int ImguiBigPictureBridge_GetRendererShutdownCalls(void) { return g_renderer_shutdown_calls; }
int ImguiBigPictureBridge_GetRendererNewFrameCalls(void) { return g_renderer_newframe_calls; }
int ImguiBigPictureBridge_GetRendererRenderCalls(void) { return g_renderer_render_calls; }
int ImguiBigPictureBridge_GetRendererSetupRenderStateCalls(void) { return g_renderer_setup_render_state_calls; }
int ImguiBigPictureBridge_GetRendererGeometryRawCalls(void) { return g_renderer_geometry_raw_calls; }
int ImguiBigPictureBridge_GetRendererCreateFontsCalls(void) { return g_renderer_create_fonts_calls; }
int ImguiBigPictureBridge_GetRendererDestroyFontsCalls(void) { return g_renderer_destroy_fonts_calls; }
int ImguiBigPictureBridge_GetRendererCreateDeviceCalls(void) { return g_renderer_create_device_calls; }
int ImguiBigPictureBridge_GetRendererDestroyDeviceCalls(void) { return g_renderer_destroy_device_calls; }
int ImguiBigPictureBridge_GetRendererFontTextureLive(void) { return g_renderer_font_texture_live; }
int ImguiBigPictureBridge_GetRendererDeviceObjectsLive(void) { return g_renderer_device_objects_live; }

int ImguiBigPictureBridge_GetTextureFromBytesCalls(void) { return g_texture_from_bytes_calls; }
int ImguiBigPictureBridge_GetTextureFromFileCalls(void) { return g_texture_from_file_calls; }

int ImguiBigPictureBridge_GetLaunchCalls(void) { return g_launch_calls; }
int ImguiBigPictureBridge_GetLastLaunchHasExecutable(void) { return g_last_launch_has_executable; }
int ImguiBigPictureBridge_GetLaunchBeginCalls(void) { return g_launch_begin_calls; }
int ImguiBigPictureBridge_GetLaunchEndCalls(void) { return g_launch_end_calls; }
int ImguiBigPictureBridge_GetLaunchPollEventCalls(void) { return g_launch_poll_event_calls; }
int ImguiBigPictureBridge_GetLaunchRenderPresentCalls(void) { return g_launch_render_present_calls; }
int ImguiBigPictureBridge_GetLaunchActive(void) { return g_launch_active; }
int ImguiBigPictureBridge_GetLaunchWindowLive(void) { return g_launch_window_live; }
int ImguiBigPictureBridge_GetLaunchRendererLive(void) { return g_launch_renderer_live; }
int ImguiBigPictureBridge_GetLaunchImGuiContextLive(void) { return g_launch_imgui_context_live; }
int ImguiBigPictureBridge_GetLaunchSettingsOpenCount(void) { return g_launch_settings_open_count; }
int ImguiBigPictureBridge_GetLaunchSeqBegin(void) { return g_launch_seq_begin; }
int ImguiBigPictureBridge_GetLaunchSeqInitSDL3(void) { return g_launch_seq_init_sdl3; }
int ImguiBigPictureBridge_GetLaunchSeqInitRenderer(void) { return g_launch_seq_init_renderer; }
int ImguiBigPictureBridge_GetLaunchSeqShutdownRenderer(void) { return g_launch_seq_shutdown_renderer; }
int ImguiBigPictureBridge_GetLaunchSeqShutdownSDL3(void) { return g_launch_seq_shutdown_sdl3; }
int ImguiBigPictureBridge_GetLaunchSeqDestroyImGui(void) { return g_launch_seq_destroy_imgui; }
int ImguiBigPictureBridge_GetLaunchSeqEnd(void) { return g_launch_seq_end; }
int ImguiBigPictureBridge_GetForceWindowCreateFail(void) { return g_force_window_create_fail; }
int ImguiBigPictureBridge_GetForceSDL3InitFail(void) { return g_force_sdl3_init_fail; }
int ImguiBigPictureBridge_GetForceRendererInitFail(void) { return g_force_renderer_init_fail; }
