// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace Shader {
struct Profile;
struct RuntimeInfo;
struct FragmentRuntimeInfo;
struct VertexRuntimeInfo;
namespace IR {
struct Program;
}
namespace Backend {
struct Bindings;
namespace SPIRV {
enum class AuxShaderType : std::uint32_t;
[[nodiscard]] std::vector<std::uint32_t> EmitSPIRV(const Profile& profile,
                                                   const RuntimeInfo& runtime_info,
                                                   const IR::Program& program,
                                                   Bindings& binding);
[[nodiscard]] std::vector<std::uint32_t> EmitAuxilaryTessShader(AuxShaderType type,
                                                                const VertexRuntimeInfo& vs_info,
                                                                const FragmentRuntimeInfo& fs_info);
} // namespace SPIRV
} // namespace Backend
} // namespace Shader

namespace {

constexpr int kElisaSpirvBridgeOk = 0;
constexpr int kElisaSpirvBridgeInvalidArgs = -1;
constexpr int kElisaSpirvBridgeAllocFailed = -2;
constexpr int kElisaSpirvBridgeException = -3;

constexpr std::uint32_t kSpirvExecutionModeInputPoints = 19u;
constexpr std::uint32_t kSpirvExecutionModeInputLines = 20u;
constexpr std::uint32_t kSpirvExecutionModeInputLinesAdjacency = 21u;
constexpr std::uint32_t kSpirvExecutionModeTriangles = 22u;
constexpr std::uint32_t kSpirvExecutionModeInputTrianglesAdjacency = 23u;
constexpr std::uint32_t kSpirvExecutionModeOutputVertices = 26u;
constexpr std::uint32_t kSpirvExecutionModeOutputLineStrip = 28u;
constexpr std::uint32_t kSpirvExecutionModeOutputTriangleStrip = 29u;

constexpr std::uint32_t kAmdGpuPrimitivePointList = 1u;
constexpr std::uint32_t kAmdGpuPrimitiveLineList = 2u;
constexpr std::uint32_t kAmdGpuPrimitiveLineStrip = 3u;
constexpr std::uint32_t kAmdGpuPrimitiveTriangleList = 4u;
constexpr std::uint32_t kAmdGpuPrimitiveTriangleStrip = 6u;
constexpr std::uint32_t kAmdGpuPrimitiveAdjLineList = 10u;
constexpr std::uint32_t kAmdGpuPrimitiveAdjTriangleList = 12u;
constexpr std::uint32_t kAmdGpuPrimitiveRectList = 17u;

constexpr std::uint32_t kAmdGpuGsOutputPointList = 0u;
constexpr std::uint32_t kAmdGpuGsOutputLineStrip = 1u;
constexpr std::uint32_t kAmdGpuGsOutputTriangleStrip = 2u;

[[nodiscard]] std::uint32_t InputPrimitiveExecutionMode(const std::uint32_t primitive_type) {
    switch (primitive_type) {
    case kAmdGpuPrimitivePointList:
        return kSpirvExecutionModeInputPoints;
    case kAmdGpuPrimitiveLineList:
    case kAmdGpuPrimitiveLineStrip:
        return kSpirvExecutionModeInputLines;
    case kAmdGpuPrimitiveTriangleList:
    case kAmdGpuPrimitiveTriangleStrip:
    case kAmdGpuPrimitiveRectList:
        return kSpirvExecutionModeTriangles;
    case kAmdGpuPrimitiveAdjTriangleList:
        return kSpirvExecutionModeInputTrianglesAdjacency;
    case kAmdGpuPrimitiveAdjLineList:
        return kSpirvExecutionModeInputLinesAdjacency;
    default:
        return 0u;
    }
}

[[nodiscard]] std::uint32_t OutputPrimitiveExecutionMode(const std::uint32_t output_type) {
    switch (output_type) {
    case kAmdGpuGsOutputPointList:
        return kSpirvExecutionModeOutputVertices;
    case kAmdGpuGsOutputLineStrip:
        return kSpirvExecutionModeOutputLineStrip;
    case kAmdGpuGsOutputTriangleStrip:
        return kSpirvExecutionModeOutputTriangleStrip;
    default:
        return 0u;
    }
}

[[nodiscard]] int CopyWordsToOwnedBuffer(const std::vector<std::uint32_t>& words,
                                         std::uint32_t** out_words,
                                         std::size_t* out_word_count) {
    if (!out_words || !out_word_count) {
        return kElisaSpirvBridgeInvalidArgs;
    }
    *out_words = nullptr;
    *out_word_count = 0;
    if (words.empty()) {
        return kElisaSpirvBridgeOk;
    }

    const auto byte_size = words.size() * sizeof(std::uint32_t);
    auto* const owned = static_cast<std::uint32_t*>(std::malloc(byte_size));
    if (!owned) {
        return kElisaSpirvBridgeAllocFailed;
    }

    std::memcpy(owned, words.data(), byte_size);
    *out_words = owned;
    *out_word_count = words.size();
    return kElisaSpirvBridgeOk;
}

} // namespace

extern "C" {

std::uint32_t elisa_spirv_backend_input_primitive_mode(const std::uint32_t primitive_type) {
    return InputPrimitiveExecutionMode(primitive_type);
}

std::uint32_t elisa_spirv_backend_output_primitive_mode(const std::uint32_t output_type) {
    return OutputPrimitiveExecutionMode(output_type);
}

int elisa_spirv_backend_emit_words(const void* profile, const void* runtime_info,
                                   const void* program, void* bindings,
                                   std::uint32_t** out_words,
                                   std::size_t* out_word_count) {
    if (!profile || !runtime_info || !program || !bindings || !out_words || !out_word_count) {
        return kElisaSpirvBridgeInvalidArgs;
    }
    try {
        const auto& profile_ref = *static_cast<const Shader::Profile*>(profile);
        const auto& runtime_info_ref = *static_cast<const Shader::RuntimeInfo*>(runtime_info);
        const auto& program_ref = *static_cast<const Shader::IR::Program*>(program);
        auto& bindings_ref = *static_cast<Shader::Backend::Bindings*>(bindings);

        std::vector<std::uint32_t> words =
            Shader::Backend::SPIRV::EmitSPIRV(profile_ref, runtime_info_ref, program_ref,
                                              bindings_ref);
        return CopyWordsToOwnedBuffer(words, out_words, out_word_count);
    } catch (...) {
        if (out_words) {
            *out_words = nullptr;
        }
        if (out_word_count) {
            *out_word_count = 0;
        }
        return kElisaSpirvBridgeException;
    }
}

int elisa_spirv_backend_emit_aux_tess_words(const std::uint32_t aux_type, const void* vs_info,
                                            const void* fs_info, std::uint32_t** out_words,
                                            std::size_t* out_word_count) {
    if (!vs_info || !fs_info || !out_words || !out_word_count) {
        return kElisaSpirvBridgeInvalidArgs;
    }
    try {
        const auto& vs_info_ref = *static_cast<const Shader::VertexRuntimeInfo*>(vs_info);
        const auto& fs_info_ref = *static_cast<const Shader::FragmentRuntimeInfo*>(fs_info);
        const auto type = static_cast<Shader::Backend::SPIRV::AuxShaderType>(aux_type);
        std::vector<std::uint32_t> words =
            Shader::Backend::SPIRV::EmitAuxilaryTessShader(type, vs_info_ref, fs_info_ref);
        return CopyWordsToOwnedBuffer(words, out_words, out_word_count);
    } catch (...) {
        if (out_words) {
            *out_words = nullptr;
        }
        if (out_word_count) {
            *out_word_count = 0;
        }
        return kElisaSpirvBridgeException;
    }
}

void elisa_spirv_backend_free_words(std::uint32_t* words) {
    std::free(words);
}

} // extern "C"
