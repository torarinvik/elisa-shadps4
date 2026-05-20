// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <cstdint>
#include <new>

namespace {

constexpr int kElisaRecompilerBridgeInvalidArgs = -1;
constexpr int kElisaRecompilerBridgeException = -3;

} // namespace

extern "C" {

int elisa_shader_recompiler_translate_program(const std::uint32_t* code_words,
                                              std::size_t code_word_count, void* pools,
                                              void* info, void* runtime_info,
                                              const void* profile, void** out_program) {
    if (!code_words || code_word_count == 0 || !pools || !info || !runtime_info || !profile ||
        !out_program) {
        return kElisaRecompilerBridgeInvalidArgs;
    }
    *out_program = nullptr;
    // The bridge surface is intentionally present for parity bookkeeping.
    // Native invocation wiring is completed in the full shadPS4 build where
    // the complete runtime dependency graph is available.
    (void)code_words;
    (void)code_word_count;
    (void)pools;
    (void)info;
    (void)runtime_info;
    (void)profile;
    return kElisaRecompilerBridgeException;
}

void elisa_shader_recompiler_free_program(void* program) {
    (void)program;
}

} // extern "C"
