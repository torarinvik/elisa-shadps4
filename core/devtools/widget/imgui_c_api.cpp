// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "imgui_c_api.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "imgui/imgui_std.h"

extern "C" {

void elisa_imgui_set_next_window_size(float x, float y, int cond) {
    ImGui::SetNextWindowSize(ImVec2{x, y}, static_cast<ImGuiCond>(cond));
}

void elisa_imgui_set_window_pos(float x, float y) {
    ImGui::SetWindowPos(ImVec2{x, y});
}

void elisa_imgui_set_window_size(float x, float y) {
    ImGui::SetWindowSize(ImVec2{x, y});
}

ElisaImGuiVec2 elisa_imgui_get_window_pos() {
    const ImVec2 value = ImGui::GetWindowPos();
    return ElisaImGuiVec2{value.x, value.y};
}

bool elisa_imgui_begin(const char* name, bool* open, int flags) {
    return ImGui::Begin(name, open, static_cast<ImGuiWindowFlags>(flags));
}

void elisa_imgui_end() {
    ImGui::End();
}

bool elisa_imgui_begin_child(const char* id, float x, float y, bool border, int flags) {
    return ImGui::BeginChild(id, ImVec2{x, y}, border, static_cast<ImGuiWindowFlags>(flags));
}

void elisa_imgui_end_child() {
    ImGui::EndChild();
}

bool elisa_imgui_begin_table(const char* id, int columns, int flags) {
    return ImGui::BeginTable(id, columns, static_cast<ImGuiTableFlags>(flags));
}

void elisa_imgui_end_table() {
    ImGui::EndTable();
}

void elisa_imgui_table_setup_column(const char* label, int flags, float init_width_or_weight) {
    ImGui::TableSetupColumn(label, static_cast<ImGuiTableColumnFlags>(flags),
                            init_width_or_weight);
}

void elisa_imgui_table_headers_row() {
    ImGui::TableHeadersRow();
}

void elisa_imgui_table_next_row() {
    ImGui::TableNextRow();
}

void elisa_imgui_table_next_column() {
    ImGui::TableNextColumn();
}

void elisa_imgui_table_set_column_index(int column) {
    ImGui::TableSetColumnIndex(column);
}

void elisa_imgui_text_unformatted(const char* text) {
    ImGui::TextUnformatted(text);
}

void elisa_imgui_text(const char* fmt) {
    ImGui::TextUnformatted(fmt);
}

void elisa_imgui_text_colored(float r, float g, float b, float a, const char* text) {
    ImGui::TextColored(ImVec4{r, g, b, a}, "%s", text);
}

bool elisa_imgui_button(const char* label) {
    return ImGui::Button(label);
}

bool elisa_imgui_checkbox(const char* label, bool* value) {
    return ImGui::Checkbox(label, value);
}

bool elisa_imgui_input_text(const char* label, char* buffer, uintptr_t buffer_size, int flags) {
    return ImGui::InputText(label, buffer, static_cast<size_t>(buffer_size),
                            static_cast<ImGuiInputTextFlags>(flags));
}

bool elisa_imgui_tree_node(const void* id, const char* label) {
    return ImGui::TreeNode(id, "%s", label);
}

void elisa_imgui_tree_pop() {
    ImGui::TreePop();
}

bool elisa_imgui_selectable(const char* label, bool selected, int flags, float x, float y) {
    return ImGui::Selectable(label, selected, static_cast<ImGuiSelectableFlags>(flags),
                             ImVec2{x, y});
}

bool elisa_imgui_collapsing_header(const char* label, int flags) {
    return ImGui::CollapsingHeader(label, static_cast<ImGuiTreeNodeFlags>(flags));
}

void elisa_imgui_same_line(float offset_from_start_x, float spacing) {
    ImGui::SameLine(offset_from_start_x, spacing);
}

void elisa_imgui_separator() {
    ImGui::Separator();
}

void elisa_imgui_separator_text(const char* label) {
    ImGui::SeparatorText(label);
}

void elisa_imgui_push_id_int(int id) {
    ImGui::PushID(id);
}

void elisa_imgui_push_id_ptr(const void* id) {
    ImGui::PushID(id);
}

void elisa_imgui_pop_id() {
    ImGui::PopID();
}

void elisa_imgui_push_style_color_u32(int idx, uint32_t color) {
    ImGui::PushStyleColor(static_cast<ImGuiCol>(idx), color);
}

void elisa_imgui_pop_style_color(int count) {
    ImGui::PopStyleColor(count);
}

void elisa_imgui_push_style_var_vec2(int idx, float x, float y) {
    ImGui::PushStyleVar(static_cast<ImGuiStyleVar>(idx), ImVec2{x, y});
}

void elisa_imgui_pop_style_var(int count) {
    ImGui::PopStyleVar(count);
}

float elisa_imgui_get_text_line_height() {
    return ImGui::GetTextLineHeight();
}

float elisa_imgui_get_text_line_height_with_spacing() {
    return ImGui::GetTextLineHeightWithSpacing();
}

float elisa_imgui_get_frame_height_with_spacing() {
    return ImGui::GetFrameHeightWithSpacing();
}

ElisaImGuiVec2 elisa_imgui_get_content_region_avail() {
    const ImVec2 value = ImGui::GetContentRegionAvail();
    return ElisaImGuiVec2{value.x, value.y};
}

ElisaImGuiVec2 elisa_imgui_get_cursor_screen_pos() {
    const ImVec2 value = ImGui::GetCursorScreenPos();
    return ElisaImGuiVec2{value.x, value.y};
}

void elisa_imgui_set_cursor_pos_x(float x) {
    ImGui::SetCursorPosX(x);
}

void elisa_imgui_dummy(float x, float y) {
    ImGui::Dummy(ImVec2{x, y});
}

bool elisa_imgui_is_item_hovered() {
    return ImGui::IsItemHovered();
}

bool elisa_imgui_is_item_active() {
    return ImGui::IsItemActive();
}

bool elisa_imgui_is_mouse_clicked(int button) {
    return ImGui::IsMouseClicked(button);
}

bool elisa_imgui_is_mouse_released(int button) {
    return ImGui::IsMouseReleased(button);
}

bool elisa_imgui_begin_popup(const char* id) {
    return ImGui::BeginPopup(id);
}

void elisa_imgui_end_popup() {
    ImGui::EndPopup();
}

void elisa_imgui_open_popup(const char* id) {
    ImGui::OpenPopup(id);
}

void elisa_imgui_set_next_item_width(float width) {
    ImGui::SetNextItemWidth(width);
}

bool elisa_imgui_begin_tooltip() {
    return ImGui::BeginTooltip();
}

void elisa_imgui_end_tooltip() {
    ImGui::EndTooltip();
}

const char* elisa_imgui_get_clipboard_text() {
    return ImGui::GetClipboardText();
}

void elisa_imgui_set_clipboard_text(const char* text) {
    ImGui::SetClipboardText(text);
}

float elisa_imgui_get_time() {
    return static_cast<float>(ImGui::GetTime());
}

bool elisa_imgui_current_window_skip_items() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    return window != nullptr && window->SkipItems;
}

ElisaImGuiVec2 elisa_imgui_get_current_window_size() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr) {
        return ElisaImGuiVec2{0.0f, 0.0f};
    }
    return ElisaImGuiVec2{window->Size.x, window->Size.y};
}

ElisaImGuiVec2 elisa_imgui_get_current_window_pos() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window == nullptr) {
        return ElisaImGuiVec2{0.0f, 0.0f};
    }
    return ElisaImGuiVec2{window->Pos.x, window->Pos.y};
}

static ImFont* GetIndexedFont(int font_index) {
    ImGuiIO& io = ImGui::GetIO();
    if (font_index >= 0 && font_index < io.Fonts->Fonts.Size) {
        return io.Fonts->Fonts[font_index];
    }
    return io.FontDefault ? io.FontDefault : ImGui::GetFont();
}

float elisa_imgui_font_size(int font_index) {
    ImFont* font = GetIndexedFont(font_index);
    return font != nullptr ? font->FontSize : 0.0f;
}

ElisaImGuiVec2 elisa_imgui_font_calc_text_size(int font_index, float font_size, const char* text) {
    ImFont* font = GetIndexedFont(font_index);
    if (font == nullptr || text == nullptr) {
        return ElisaImGuiVec2{0.0f, 0.0f};
    }
    const ImVec2 size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, nullptr, nullptr);
    return ElisaImGuiVec2{size.x, size.y};
}

float elisa_imgui_font_glyph_advance(int font_index, uint32_t codepoint, float scale) {
    ImFont* font = GetIndexedFont(font_index);
    if (font == nullptr) {
        return 0.0f;
    }
    const ImFontGlyph* glyph = font->FindGlyph(static_cast<ImWchar>(codepoint));
    const float advance = glyph != nullptr ? glyph->AdvanceX : font->FallbackAdvanceX;
    return advance * scale;
}

void elisa_imgui_window_draw_text_font(int font_index, float font_size, float x, float y, uint32_t color,
                                       const char* text) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImFont* font = GetIndexedFont(font_index);
    if (window == nullptr || font == nullptr || text == nullptr) {
        return;
    }
    window->DrawList->AddText(font, font_size, ImVec2{x, y}, color, text);
}

void elisa_imgui_window_draw_text_char(int font_index, float font_size, float x, float y,
                                       uint32_t color, uint32_t codepoint) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImFont* font = GetIndexedFont(font_index);
    if (window == nullptr || font == nullptr) {
        return;
    }
    char buffer[8]{};
    int len = ImTextCharToUtf8(buffer, static_cast<unsigned int>(codepoint));
    if (len <= 0) {
        return;
    }
    window->DrawList->AddText(font, font_size, ImVec2{x, y}, color, buffer, buffer + len);
}

} // extern "C"
