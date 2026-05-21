// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <cstdint>
#include <new>

namespace {

constexpr int kElisaRecompilerBridgeInvalidArgs = -1;
constexpr int kElisaRecompilerBridgeAllocFailed = -2;
constexpr int kElisaRecompilerBridgeOk = 0;

struct ElisaRecompilerProgramHandle {
    std::uint64_t magic;
    std::uint32_t first_word;
    std::uint32_t word_count;
};

} // namespace

extern "C" {

int elisa_shader_recompiler_translate_program(const std::uint32_t* code_words,
                                              std::size_t code_word_count, void* pools,
                                              void* info, void* runtime_info,
                                              const void* profile, void** out_program) {
    if (out_program) {
        *out_program = nullptr;
    }
    if (!code_words || code_word_count == 0 || !pools || !info || !runtime_info || !profile ||
        !out_program) {
        return kElisaRecompilerBridgeInvalidArgs;
    }
    auto* handle = new (std::nothrow) ElisaRecompilerProgramHandle{};
    if (!handle) {
        *out_program = nullptr;
        return kElisaRecompilerBridgeAllocFailed;
    }
    handle->magic = 0x454c49534150524full; // "ELISAPRO"
    handle->first_word = code_words[0];
    handle->word_count = static_cast<std::uint32_t>(code_word_count);
    *out_program = handle;
    (void)pools;
    (void)info;
    (void)runtime_info;
    (void)profile;
    return kElisaRecompilerBridgeOk;
}

void elisa_shader_recompiler_free_program(void* program) {
    auto* handle = static_cast<ElisaRecompilerProgramHandle*>(program);
    delete handle;
}

} // extern "C"
