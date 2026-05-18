//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/memory.h"

extern "C" {

typedef struct ElisaMemoryMapRow {
    uint64_t base;
    uint64_t size;
    uint32_t prot;
    uint32_t kind;
    int pooled;
    int exec;
    const char* type_name;
    const char* prot_name;
    const char* name;
} ElisaMemoryMapRow;

size_t elisa_devtools_memory_map_snapshot_vma(ElisaMemoryMapRow* rows, size_t capacity);
size_t elisa_devtools_memory_map_snapshot_dmem(ElisaMemoryMapRow* rows, size_t capacity);

}

namespace Core::Devtools::Widget {

class MemoryMapViewer {
    struct Iterator {
        bool is_vma;
        struct {
            MemoryManager::PhysMap::iterator it;
            MemoryManager::PhysMap::iterator end;
        } dmem;
        struct {
            MemoryManager::VMAMap::iterator it;
            MemoryManager::VMAMap::iterator end;
        } vma;

        bool DrawLine();
    };

    bool showing_vma = true;

public:
    bool open = false;

    static size_t SnapshotVma(ElisaMemoryMapRow* rows, size_t capacity);
    static size_t SnapshotDmem(ElisaMemoryMapRow* rows, size_t capacity);

    void Draw();
};

} // namespace Core::Devtools::Widget
