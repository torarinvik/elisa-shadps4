// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <unordered_set>
#include <vector>

namespace {

constexpr int kElisaRecompilerBridgeInvalidArgs = -1;
constexpr int kElisaRecompilerBridgeAllocFailed = -2;
constexpr int kElisaRecompilerBridgeOk = 0;
constexpr std::uint64_t kElisaRecompilerProgramMagicLive = 0x454c49534150524full;  // "ELISAPRO"
constexpr std::uint64_t kElisaRecompilerProgramMagicFreed = 0x454c495341505246ull; // "ELISAPRF"
constexpr std::int32_t kElisaRecompilerProgramStatusInvalid = -1;
constexpr std::int32_t kElisaRecompilerProgramStatusPlaceholder = 1;
constexpr std::int32_t kElisaRecompilerProgramStatusFreed = 2;
constexpr std::int32_t kElisaRecompilerProgramStatusTranslated = 3;
constexpr std::int32_t kElisaRecompilerProgramStatusFailedWithDiagnostic = 4;

// Diagnostic codes (stored in handle->diagnostic_code when status == FailedWithDiagnostic)
constexpr std::int32_t kElisaRecompilerDiagnosticNone = 0;
constexpr std::int32_t kElisaRecompilerDiagnosticRealLinkNotAvailable = 1;
constexpr std::int32_t kElisaRecompilerDiagnosticException = 2;

struct ElisaRecompilerProgramHandle {
    std::uint64_t magic;
    std::uint64_t input_hash;
    std::uint64_t word_count;
    std::uint32_t stage_type;
    std::uint32_t has_stage_type;
    std::uint64_t output_byte_size;
    std::uint64_t output_hash;
    std::int32_t status;
    std::int32_t diagnostic_code;
    // Owned SPIR-V output bytes. Empty when status == Placeholder or FailedWithDiagnostic.
    std::vector<std::uint8_t> output_data;
};

std::mutex g_handle_mutex;
std::unordered_set<const void*> g_live_handles;
std::unordered_set<const void*> g_freed_handles;

[[nodiscard]] std::uint64_t FnvHash64Words(const std::uint32_t* code_words,
                                           const std::size_t code_word_count) {
    constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    std::uint64_t hash = kFnvOffsetBasis;
    hash ^= static_cast<std::uint64_t>(code_word_count);
    hash *= kFnvPrime;
    for (std::size_t index = 0; index < code_word_count; ++index) {
        hash ^= static_cast<std::uint64_t>(code_words[index]);
        hash *= kFnvPrime;
    }
    return hash;
}

[[nodiscard]] std::uint64_t FnvHash64Bytes(const std::uint8_t* data, const std::size_t size) {
    constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    if (!data || size == 0) {
        return 0u;
    }
    std::uint64_t hash = kFnvOffsetBasis;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= static_cast<std::uint64_t>(data[i]);
        hash *= kFnvPrime;
    }
    return hash;
}

[[nodiscard]] bool IsLiveHandle(const ElisaRecompilerProgramHandle* handle) {
    return handle && g_live_handles.find(handle) != g_live_handles.end() &&
           handle->magic == kElisaRecompilerProgramMagicLive;
}

[[nodiscard]] bool IsFreedHandle(const ElisaRecompilerProgramHandle* handle) {
    return handle && g_freed_handles.find(handle) != g_freed_handles.end();
}

void MarkHandleFreed(ElisaRecompilerProgramHandle* handle) {
    g_live_handles.erase(handle);
    g_freed_handles.insert(handle);
    delete handle;
}

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
    handle->magic = kElisaRecompilerProgramMagicLive;
    handle->input_hash = FnvHash64Words(code_words, code_word_count);
    handle->word_count = static_cast<std::uint64_t>(code_word_count);
    handle->stage_type = 0u;
    handle->has_stage_type = 0u;
    handle->output_byte_size = 0u;
    handle->output_hash = 0u;
    handle->diagnostic_code = kElisaRecompilerDiagnosticRealLinkNotAvailable;
    // NOTE: The real shadPS4 TranslateProgram + EmitSPIRV path requires linking ~62 cpp files
    // from shader_recompiler/ plus sirit, boost, fmt, and magic_enum. Until the build system
    // is extended to include that full dependency chain, the bridge reports Placeholder status.
    // The pools/info/runtime_info/profile parameters are received but not forwarded.
    // Retirement: replace when the Elisa build system links the full recompiler chain and
    // the native Elisa backend emits SPIR-V directly.
    handle->status = kElisaRecompilerProgramStatusPlaceholder;
    handle->output_data.clear();
    (void)pools;
    (void)info;
    (void)runtime_info;
    (void)profile;
    {
        std::lock_guard<std::mutex> lock{g_handle_mutex};
        g_freed_handles.erase(handle);
        g_live_handles.insert(handle);
    }
    *out_program = handle;
    return kElisaRecompilerBridgeOk;
}

void elisa_shader_recompiler_free_program(void* program) {
    auto* handle = static_cast<ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (!IsLiveHandle(handle)) {
        return;
    }
    MarkHandleFreed(handle);
}

std::uint64_t elisa_shader_recompiler_program_magic(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    if (!handle) {
        return 0u;
    }
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle)) {
        return kElisaRecompilerProgramMagicFreed;
    }
    if (!IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->magic;
}

std::uint64_t elisa_shader_recompiler_program_input_hash(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->input_hash;
}

std::uint64_t elisa_shader_recompiler_program_input_word_count(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->word_count;
}

std::uint32_t elisa_shader_recompiler_program_stage_type(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->stage_type;
}

std::uint32_t elisa_shader_recompiler_program_has_stage_type(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->has_stage_type;
}

std::uint64_t elisa_shader_recompiler_program_output_byte_size(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->output_byte_size;
}

// FNV-1a hash over the raw SPIR-V output bytes. Returns 0 if the handle is freed,
// invalid, or has no output (status == Placeholder or FailedWithDiagnostic).
std::uint64_t elisa_shader_recompiler_program_output_hash(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0u;
    }
    return handle->output_hash;
}

// Returns status as int64_t so Elisa's 64-bit int comparison works correctly for negative
// values (e.g. STATUS_INVALID = -1). Returning int32_t would leave upper bits undefined
// on ARM64, causing -1 to compare incorrectly against Elisa's int64(-1).
std::int64_t elisa_shader_recompiler_program_status(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    if (!handle) {
        return static_cast<std::int64_t>(kElisaRecompilerProgramStatusInvalid);
    }
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle)) {
        return static_cast<std::int64_t>(kElisaRecompilerProgramStatusFreed);
    }
    if (!IsLiveHandle(handle)) {
        return static_cast<std::int64_t>(kElisaRecompilerProgramStatusInvalid);
    }
    return static_cast<std::int64_t>(handle->status);
}

// Returns a diagnostic code when status == FailedWithDiagnostic, 0 otherwise.
// kElisaRecompilerDiagnosticRealLinkNotAvailable (1): real TranslateProgram not linked.
// kElisaRecompilerDiagnosticException (2): C++ exception caught during translation attempt.
// Returns int64_t for consistency with elisa_shader_recompiler_program_status.
std::int64_t elisa_shader_recompiler_program_diagnostic_code(const void* program) {
    const auto* handle = static_cast<const ElisaRecompilerProgramHandle*>(program);
    if (!handle) {
        return 0;
    }
    std::lock_guard<std::mutex> lock{g_handle_mutex};
    if (IsFreedHandle(handle) || !IsLiveHandle(handle)) {
        return 0;
    }
    return static_cast<std::int64_t>(handle->diagnostic_code);
}

} // extern "C"
