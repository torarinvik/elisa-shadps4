//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <algorithm>
#include <array>
#include <cstring>
#include <cstdio>
#include <string>
#include <string_view>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "core/debug_state.h"
#include "core/memory.h"
#include "memory_map.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

bool MemoryMapViewer::Iterator::DrawLine() {
    if (is_vma) {
        if (vma.it == vma.end) {
            return false;
        }
        auto m = vma.it->second;
        if (m.type == VMAType::Free) {
            ++vma.it;
            return DrawLine();
        }
        TableNextColumn();
        Text("%" PRIXPTR, m.base);
        TableNextColumn();
        Text("%" PRIX64, m.size);
        TableNextColumn();
        Text("%s", magic_enum::enum_name(m.type).data());
        TableNextColumn();
        Text("%s", magic_enum::enum_name(m.prot).data());
        TableNextColumn();
        if (True(m.prot & MemoryProt::CpuExec)) {
            Text("X");
        }
        TableNextColumn();
        Text("%s", m.name.c_str());
        ++vma.it;
        return true;
    }
    if (dmem.it == dmem.end) {
        return false;
    }
    auto m = dmem.it->second;
    if (m.dma_type == PhysicalMemoryType::Free) {
        ++dmem.it;
        return DrawLine();
    }
    TableNextColumn();
    Text("%" PRIXPTR, m.base);
    TableNextColumn();
    Text("%" PRIX64, m.size);
    TableNextColumn();
    auto type = static_cast<::Libraries::Kernel::MemoryTypes>(m.memory_type);
    Text("%s", magic_enum::enum_name(type).data());
    TableNextColumn();
    Text("%d",
         m.dma_type == PhysicalMemoryType::Pooled || m.dma_type == PhysicalMemoryType::Committed);
    ++dmem.it;
    return true;
}

void MemoryMapViewer::Draw() {
    SetNextWindowSize({600.0f, 500.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("Memory map", &open)) {
        End();
        return;
    }

    auto mem = Memory::Instance();
    std::scoped_lock lck{mem->mutex};

    {
        bool next_showing_vma = showing_vma;
        if (showing_vma) {
            PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
        }
        if (Button("VMem")) {
            next_showing_vma = true;
        }
        if (showing_vma) {
            PopStyleColor();
        }
        SameLine();
        if (!showing_vma) {
            PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
        }
        if (Button("DMem")) {
            next_showing_vma = false;
        }
        if (!showing_vma) {
            PopStyleColor();
        }
        showing_vma = next_showing_vma;
    }

    Iterator it{};
    if (showing_vma) {
        it.is_vma = true;
        it.vma.it = mem->vma_map.begin();
        it.vma.end = mem->vma_map.end();
    } else {
        it.is_vma = false;
        it.dmem.it = mem->dmem_map.begin();
        it.dmem.end = mem->dmem_map.end();
    }

    if (BeginTable("memory_view_table", showing_vma ? 6 : 4,
                   ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg |
                       ImGuiTableFlags_SizingFixedFit)) {
        if (showing_vma) {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableSetupColumn("Prot");
            TableSetupColumn("Is Exec");
            TableSetupColumn("Name");
        } else {
            TableSetupColumn("Address");
            TableSetupColumn("Size");
            TableSetupColumn("Type");
            TableSetupColumn("Pooled");
        }
        TableHeadersRow();

        while (it.DrawLine())
            ;
        EndTable();
    }

    End();
}

} // namespace Core::Devtools::Widget

namespace {

thread_local std::array<std::string, 512> g_elisa_memory_map_type_names;
thread_local std::array<std::string, 512> g_elisa_memory_map_prot_names;
thread_local std::array<std::string, 512> g_elisa_memory_map_names;

const char* VmaTypeName(Core::VMAType type) {
    switch (type) {
    case Core::VMAType::Free:
        return "Free";
    case Core::VMAType::Reserved:
        return "Reserved";
    case Core::VMAType::Direct:
        return "Direct";
    case Core::VMAType::Flexible:
        return "Flexible";
    case Core::VMAType::Pooled:
        return "Pooled";
    case Core::VMAType::PoolReserved:
        return "PoolReserved";
    case Core::VMAType::Stack:
        return "Stack";
    case Core::VMAType::Code:
        return "Code";
    case Core::VMAType::File:
        return "File";
    }
    return "Unknown";
}

const char* PhysicalMemoryTypeName(Core::PhysicalMemoryType type) {
    switch (type) {
    case Core::PhysicalMemoryType::Free:
        return "Free";
    case Core::PhysicalMemoryType::Allocated:
        return "Allocated";
    case Core::PhysicalMemoryType::Mapped:
        return "Mapped";
    case Core::PhysicalMemoryType::Pooled:
        return "Pooled";
    case Core::PhysicalMemoryType::Committed:
        return "Committed";
    case Core::PhysicalMemoryType::Flexible:
        return "Flexible";
    }
    return "Unknown";
}

std::string FormatMemoryProt(Core::MemoryProt prot) {
    const bool read = True(prot & Core::MemoryProt::CpuRead);
    const bool write = True(prot & Core::MemoryProt::CpuWrite);
    const bool exec = True(prot & Core::MemoryProt::CpuExec);
    char out[4];
    std::snprintf(out, sizeof(out), "%c%c%c", read ? 'R' : '-', write ? 'W' : '-',
                  exec ? 'X' : '-');
    return out;
}

} // namespace

extern "C" {

size_t elisa_devtools_memory_map_snapshot_vma(ElisaMemoryMapRow* rows, size_t capacity) {
    return Core::Devtools::Widget::MemoryMapViewer::SnapshotVma(rows, capacity);
}

size_t elisa_devtools_memory_map_snapshot_dmem(ElisaMemoryMapRow* rows, size_t capacity) {
    return Core::Devtools::Widget::MemoryMapViewer::SnapshotDmem(rows, capacity);
}

} // extern "C"

namespace Core::Devtools::Widget {

size_t MemoryMapViewer::SnapshotVma(ElisaMemoryMapRow* rows, size_t capacity) {
    if (rows == nullptr || capacity == 0) {
        return 0;
    }
    auto* mem = Memory::Instance();
    std::scoped_lock lck{mem->mutex};
    size_t count = 0;
    for (const auto& [_, area] : mem->vma_map) {
        if (area.type == VMAType::Free) {
            continue;
        }
        if (count >= capacity) {
            break;
        }
        auto& row = rows[count++];
        row.base = area.base;
        row.size = area.size;
        row.prot = static_cast<uint32_t>(area.prot);
        row.kind = static_cast<uint32_t>(area.type);
        row.pooled = 0;
        row.exec = True(area.prot & Core::MemoryProt::CpuExec) ? 1 : 0;
        g_elisa_memory_map_type_names[count - 1] = VmaTypeName(area.type);
        g_elisa_memory_map_prot_names[count - 1] = FormatMemoryProt(area.prot);
        g_elisa_memory_map_names[count - 1] = area.name;
        row.type_name = g_elisa_memory_map_type_names[count - 1].c_str();
        row.prot_name = g_elisa_memory_map_prot_names[count - 1].c_str();
        row.name = g_elisa_memory_map_names[count - 1].c_str();
    }
    return count;
}

size_t MemoryMapViewer::SnapshotDmem(ElisaMemoryMapRow* rows, size_t capacity) {
    if (rows == nullptr || capacity == 0) {
        return 0;
    }
    auto* mem = Memory::Instance();
    std::scoped_lock lck{mem->mutex};
    size_t count = 0;
    for (const auto& [_, area] : mem->dmem_map) {
        if (area.dma_type == PhysicalMemoryType::Free) {
            continue;
        }
        if (count >= capacity) {
            break;
        }
        auto& row = rows[count++];
        row.base = area.base;
        row.size = area.size;
        row.prot = 0;
        row.kind = static_cast<uint32_t>(area.dma_type);
        row.pooled = area.dma_type == PhysicalMemoryType::Pooled ||
                             area.dma_type == PhysicalMemoryType::Committed
                         ? 1
                         : 0;
        row.exec = 0;
        g_elisa_memory_map_type_names[count - 1] = PhysicalMemoryTypeName(area.dma_type);
        g_elisa_memory_map_prot_names[count - 1].clear();
        g_elisa_memory_map_names[count - 1].clear();
        row.type_name = g_elisa_memory_map_type_names[count - 1].c_str();
        row.prot_name = g_elisa_memory_map_prot_names[count - 1].c_str();
        row.name = g_elisa_memory_map_names[count - 1].c_str();
    }
    return count;
}

} // namespace Core::Devtools::Widget
