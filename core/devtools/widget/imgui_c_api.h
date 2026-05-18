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

void elisa_imgui_set_next_window_size(float x, float y, int cond);
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
void elisa_imgui_pop_style_color(int count);
void elisa_imgui_push_style_var_vec2(int idx, float x, float y);
void elisa_imgui_pop_style_var(int count);
float elisa_imgui_get_text_line_height(void);
float elisa_imgui_get_text_line_height_with_spacing(void);
float elisa_imgui_get_frame_height_with_spacing(void);
ElisaImGuiVec2 elisa_imgui_get_content_region_avail(void);
ElisaImGuiVec2 elisa_imgui_get_cursor_screen_pos(void);
void elisa_imgui_set_cursor_pos_x(float x);
void elisa_imgui_dummy(float x, float y);
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

#ifdef __cplusplus
}
#endif
