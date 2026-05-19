// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ElisaImGuiVec2 {
    float x;
    float y;
} ElisaImGuiVec2;

typedef struct ElisaImGuiVec4 {
    float x;
    float y;
    float z;
    float w;
} ElisaImGuiVec4;

typedef struct ElisaImGuiIOState {
    ElisaImGuiVec2 display_size;
    float delta_time;
    bool nav_active;
} ElisaImGuiIOState;

enum {
    ELISA_IMGUI_WINDOW_FLAGS_NONE = 0,
    ELISA_IMGUI_WINDOW_FLAGS_NO_TITLE_BAR = 1 << 0,
    ELISA_IMGUI_WINDOW_FLAGS_NO_RESIZE = 1 << 1,
    ELISA_IMGUI_WINDOW_FLAGS_NO_MOVE = 1 << 2,
    ELISA_IMGUI_WINDOW_FLAGS_NO_SCROLLBAR = 1 << 3,
    ELISA_IMGUI_WINDOW_FLAGS_NO_SCROLL_WITH_MOUSE = 1 << 4,
    ELISA_IMGUI_WINDOW_FLAGS_NO_COLLAPSE = 1 << 5,
    ELISA_IMGUI_WINDOW_FLAGS_ALWAYS_AUTO_RESIZE = 1 << 6,
    ELISA_IMGUI_WINDOW_FLAGS_NO_BACKGROUND = 1 << 7,
    ELISA_IMGUI_WINDOW_FLAGS_NO_SAVED_SETTINGS = 1 << 8,
    ELISA_IMGUI_WINDOW_FLAGS_NO_MOUSE_INPUTS = 1 << 9,
    ELISA_IMGUI_WINDOW_FLAGS_MENU_BAR = 1 << 10,
    ELISA_IMGUI_WINDOW_FLAGS_HORIZONTAL_SCROLLBAR = 1 << 11,
    ELISA_IMGUI_WINDOW_FLAGS_NO_FOCUS_ON_APPEARING = 1 << 12,
    ELISA_IMGUI_WINDOW_FLAGS_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 13,
    ELISA_IMGUI_WINDOW_FLAGS_ALWAYS_VERTICAL_SCROLLBAR = 1 << 14,
    ELISA_IMGUI_WINDOW_FLAGS_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
    ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_INPUTS = 1 << 16,
    ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_FOCUS = 1 << 17,
    ELISA_IMGUI_WINDOW_FLAGS_NO_NAV = ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_INPUTS |
                                      ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_FOCUS,
    ELISA_IMGUI_WINDOW_FLAGS_NO_DECORATION = ELISA_IMGUI_WINDOW_FLAGS_NO_TITLE_BAR |
                                             ELISA_IMGUI_WINDOW_FLAGS_NO_RESIZE |
                                             ELISA_IMGUI_WINDOW_FLAGS_NO_SCROLLBAR |
                                             ELISA_IMGUI_WINDOW_FLAGS_NO_COLLAPSE,
    ELISA_IMGUI_WINDOW_FLAGS_NO_INPUTS = ELISA_IMGUI_WINDOW_FLAGS_NO_MOUSE_INPUTS |
                                         ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_INPUTS |
                                         ELISA_IMGUI_WINDOW_FLAGS_NO_NAV_FOCUS,
};

enum {
    ELISA_IMGUI_COL_TEXT = 0,
    ELISA_IMGUI_COL_WINDOW_BG = 2,
    ELISA_IMGUI_COL_BORDER = 5,
    ELISA_IMGUI_COL_BUTTON = 21,
    ELISA_IMGUI_COL_BUTTON_HOVERED = 22,
    ELISA_IMGUI_COL_BUTTON_ACTIVE = 23,
};

enum {
    ELISA_IMGUI_STYLE_VAR_WINDOW_PADDING = 2,
    ELISA_IMGUI_STYLE_VAR_WINDOW_ROUNDING = 3,
    ELISA_IMGUI_STYLE_VAR_WINDOW_BORDER_SIZE = 4,
    ELISA_IMGUI_STYLE_VAR_FRAME_ROUNDING = 11,
    ELISA_IMGUI_STYLE_VAR_FRAME_BORDER_SIZE = 13,
};

ElisaImGuiIOState elisa_imgui_get_io_state(void);
void elisa_imgui_set_next_window_pos(float x, float y, int cond);
void elisa_imgui_set_next_window_size(float x, float y, int cond);
void elisa_imgui_set_next_window_collapsed(bool collapsed, int cond);
void elisa_imgui_set_next_window_focus(void);
void elisa_imgui_set_window_pos(float x, float y);
void elisa_imgui_set_window_size(float x, float y);
ElisaImGuiVec2 elisa_imgui_get_window_pos(void);
bool elisa_imgui_begin(const char* name, bool* open, int flags);
void elisa_imgui_end(void);
bool elisa_imgui_begin_child(const char* id, float x, float y, bool border, int flags);
void elisa_imgui_end_child(void);
bool elisa_imgui_begin_table(const char* id, int columns, int flags);
void elisa_imgui_end_table(void);
void elisa_imgui_table_setup_column(const char* label, int flags, float init_width_or_weight);
void elisa_imgui_table_headers_row(void);
void elisa_imgui_table_next_row(void);
void elisa_imgui_table_next_column(void);
void elisa_imgui_table_set_column_index(int column);
void elisa_imgui_text_unformatted(const char* text);
void elisa_imgui_text(const char* fmt);
void elisa_imgui_text_colored(float r, float g, float b, float a, const char* text);
bool elisa_imgui_button(const char* label);
bool elisa_imgui_checkbox(const char* label, bool* value);
bool elisa_imgui_input_text(const char* label, char* buffer, uintptr_t buffer_size, int flags);
bool elisa_imgui_tree_node(const void* id, const char* label);
void elisa_imgui_tree_pop(void);
bool elisa_imgui_selectable(const char* label, bool selected, int flags, float x, float y);
bool elisa_imgui_collapsing_header(const char* label, int flags);
void elisa_imgui_same_line(float offset_from_start_x, float spacing);
void elisa_imgui_separator(void);
void elisa_imgui_separator_text(const char* label);
void elisa_imgui_push_id_int(int id);
void elisa_imgui_push_id_ptr(const void* id);
void elisa_imgui_pop_id(void);
void elisa_imgui_push_style_color_u32(int idx, uint32_t color);
void elisa_imgui_push_style_color_vec4(int idx, float r, float g, float b, float a);
void elisa_imgui_pop_style_color(int count);
void elisa_imgui_push_style_var_vec2(int idx, float x, float y);
void elisa_imgui_push_style_var_float(int idx, float value);
void elisa_imgui_pop_style_var(int count);
float elisa_imgui_get_text_line_height(void);
float elisa_imgui_get_text_line_height_with_spacing(void);
float elisa_imgui_get_frame_height_with_spacing(void);
ElisaImGuiVec2 elisa_imgui_get_content_region_avail(void);
ElisaImGuiVec2 elisa_imgui_get_cursor_screen_pos(void);
void elisa_imgui_set_cursor_pos(float x, float y);
void elisa_imgui_set_cursor_pos_x(float x);
void elisa_imgui_dummy(float x, float y);
ElisaImGuiVec2 elisa_imgui_calc_text_size(const char* text);
bool elisa_imgui_button_sized(const char* label, float x, float y);
void elisa_imgui_set_item_default_focus(void);
bool elisa_imgui_is_item_hovered(void);
bool elisa_imgui_is_item_active(void);
bool elisa_imgui_is_mouse_clicked(int button);
bool elisa_imgui_is_mouse_released(int button);
bool elisa_imgui_begin_popup(const char* id);
void elisa_imgui_end_popup(void);
void elisa_imgui_open_popup(const char* id);
void elisa_imgui_set_next_item_width(float width);
bool elisa_imgui_begin_tooltip(void);
void elisa_imgui_end_tooltip(void);
const char* elisa_imgui_get_clipboard_text(void);
void elisa_imgui_set_clipboard_text(const char* text);
float elisa_imgui_get_time(void);
bool elisa_imgui_current_window_skip_items(void);
ElisaImGuiVec2 elisa_imgui_get_current_window_size(void);
ElisaImGuiVec2 elisa_imgui_get_current_window_pos(void);
float elisa_imgui_font_size(int font_index);
ElisaImGuiVec2 elisa_imgui_font_calc_text_size(int font_index, float font_size, const char* text);
float elisa_imgui_font_glyph_advance(int font_index, uint32_t codepoint, float scale);
void elisa_imgui_window_draw_text_font(int font_index, float font_size, float x, float y,
                                       uint32_t color, const char* text);
void elisa_imgui_window_draw_text_char(int font_index, float font_size, float x, float y,
                                       uint32_t color, uint32_t codepoint);
uint32_t elisa_imgui_color_convert_float4_to_u32(float r, float g, float b, float a);
void elisa_imgui_window_draw_rect_filled(float min_x, float min_y, float max_x, float max_y,
                                         uint32_t color, float rounding);
void elisa_imgui_window_draw_rect_filled_multicolor(float min_x, float min_y, float max_x,
                                                    float max_y, uint32_t col_upr_left,
                                                    uint32_t col_upr_right,
                                                    uint32_t col_bot_right,
                                                    uint32_t col_bot_left);
void elisa_imgui_window_draw_rect(float min_x, float min_y, float max_x, float max_y,
                                  uint32_t color, float rounding, float thickness);
void elisa_imgui_window_draw_circle_filled(float center_x, float center_y, float radius,
                                           uint32_t color, int segments);
void elisa_imgui_window_draw_circle(float center_x, float center_y, float radius,
                                    uint32_t color, int segments, float thickness);
void elisa_imgui_window_draw_line(float a_x, float a_y, float b_x, float b_y, uint32_t color,
                                  float thickness);

#ifdef __cplusplus
}
#endif
