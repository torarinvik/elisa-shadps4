// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/object_pool.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/structured_control_flow.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/abstract_syntax_list.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/params.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"

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
constexpr std::int32_t kElisaRecompilerDiagnosticUnsupported = 1;
constexpr std::int32_t kElisaRecompilerDiagnosticException = 2;

struct ElisaRecompilerTranslateMetadata {
    std::uint32_t stage;
    std::uint32_t logical_stage;
};

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
    // Owned SPIR-V output bytes. Empty when translation is unsupported or failed.
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

[[nodiscard]] std::uint64_t FnvHash64Bytes(const std::uint8_t* bytes, const std::size_t size) {
    constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t kFnvPrime = 1099511628211ull;

    std::uint64_t hash = kFnvOffsetBasis;
    hash ^= static_cast<std::uint64_t>(size);
    hash *= kFnvPrime;
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= static_cast<std::uint64_t>(bytes[index]);
        hash *= kFnvPrime;
    }
    return hash;
}

[[nodiscard]] std::vector<std::uint8_t> SerializeTranslatedProgram(const Shader::IR::Program& program) {
    std::map<const Shader::IR::Block*, std::size_t> block_to_index;
    for (std::size_t index = 0; index < program.blocks.size(); ++index) {
        block_to_index.emplace(program.blocks[index], index);
    }

    std::map<const Shader::IR::Inst*, std::size_t> inst_to_index;
    std::size_t inst_index = 0;

    std::string output;
    output += fmt::format("stage={} logical_stage={} hash={:#018x} blocks={} instructions={}\n",
                          static_cast<std::uint32_t>(program.info.stage),
                          static_cast<std::uint32_t>(program.info.l_stage), program.info.pgm_hash,
                          program.blocks.size(), program.ins_list.size());

    for (const Shader::IR::Block* const block : program.blocks) {
        output += Shader::IR::DumpBlock(*block, block_to_index, inst_to_index, inst_index);
        output.push_back('\n');
    }

    for (const auto& node : program.syntax_list) {
        output += Shader::IR::DumpASLNode(node, block_to_index, inst_to_index);
        output.push_back('\n');
    }

    return {output.begin(), output.end()};
}

[[nodiscard]] Shader::IR::BlockList GenerateBlocks(const Shader::IR::AbstractSyntaxList& syntax_list) {
    std::size_t num_syntax_blocks{};
    for (const auto& [_, type] : syntax_list) {
        if (type == Shader::IR::AbstractSyntaxNode::Type::Block) {
            ++num_syntax_blocks;
        }
    }
    Shader::IR::BlockList blocks{};
    blocks.reserve(num_syntax_blocks);
    for (const auto& [data, type] : syntax_list) {
        if (type == Shader::IR::AbstractSyntaxNode::Type::Block) {
            blocks.push_back(data.block);
        }
    }
    return blocks;
}

[[nodiscard]] bool TryTranslateStage(const std::uint32_t raw_stage, Shader::Stage& stage) {
    switch (raw_stage) {
    case 0u:
        stage = Shader::Stage::Fragment;
        return true;
    case 1u:
        stage = Shader::Stage::Vertex;
        return true;
    case 2u:
        stage = Shader::Stage::Geometry;
        return true;
    case 3u:
        stage = Shader::Stage::Export;
        return true;
    case 4u:
        stage = Shader::Stage::Hull;
        return true;
    case 5u:
        stage = Shader::Stage::Local;
        return true;
    case 6u:
        stage = Shader::Stage::Compute;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] bool TryTranslateLogicalStage(const std::uint32_t raw_stage,
                                            Shader::LogicalStage& stage) {
    switch (raw_stage) {
    case 0u:
        stage = Shader::LogicalStage::Fragment;
        return true;
    case 1u:
        stage = Shader::LogicalStage::TessellationControl;
        return true;
    case 2u:
        stage = Shader::LogicalStage::TessellationEval;
        return true;
    case 3u:
        stage = Shader::LogicalStage::Vertex;
        return true;
    case 4u:
        stage = Shader::LogicalStage::Geometry;
        return true;
    case 5u:
        stage = Shader::LogicalStage::Compute;
        return true;
    default:
        return false;
    }
}

[[nodiscard]] Shader::Profile DefaultProfile() {
    Shader::Profile profile{};
    profile.max_ubo_size = 65536ULL;
    profile.max_viewport_width = 16384u;
    profile.max_viewport_height = 16384u;
    profile.max_shared_memory_size = 65536u;
    profile.supported_spirv = 0x00010000u;
    profile.subgroup_size = 64u;
    profile.support_int8 = true;
    profile.support_int16 = true;
    profile.support_int64 = true;
    profile.support_float16 = true;
    profile.support_float64 = false;
    profile.supports_denorm_behavior_independence = false;
    profile.supports_rounding_mode_independence = false;
    profile.support_fp16_denorm_preserve = false;
    profile.support_fp16_denorm_flush = false;
    profile.support_fp16_round_to_zero = false;
    profile.support_fp32_denorm_preserve = false;
    profile.support_fp32_denorm_flush = false;
    profile.support_fp32_round_to_zero = false;
    profile.support_fp64_denorm_preserve = false;
    profile.support_fp64_denorm_flush = false;
    profile.support_fp64_round_to_zero = false;
    profile.support_fp16_signed_zero_inf_nan_preserve = false;
    profile.support_fp32_signed_zero_inf_nan_preserve = false;
    profile.support_fp64_signed_zero_inf_nan_preserve = false;
    profile.support_legacy_vertex_attributes = false;
    profile.supports_image_load_store_lod = false;
    profile.supports_native_cube_calc = false;
    profile.supports_trinary_minmax = false;
    profile.supports_buffer_fp32_atomic_min_max = false;
    profile.supports_image_fp32_atomic_min_max = false;
    profile.supports_buffer_int64_atomics = false;
    profile.supports_shared_int64_atomics = false;
    profile.supports_workgroup_explicit_memory_layout = false;
    profile.supports_amd_shader_explicit_vertex_parameter = false;
    profile.supports_fragment_shader_barycentric = false;
    profile.supports_shader_stencil_export = false;
    profile.has_incomplete_fragment_shader_barycentric = false;
    profile.has_broken_spirv_clamp = false;
    profile.lower_left_origin_mode = false;
    profile.needs_manual_interpolation = false;
    profile.needs_lds_barriers = false;
    profile.needs_buffer_offsets = false;
    profile.needs_unorm_fixup = false;
    profile.needs_clip_distance_emulation = false;
    return profile;
}

[[nodiscard]] Shader::RuntimeInfo DefaultRuntimeInfo(const Shader::Stage stage) {
    Shader::RuntimeInfo runtime_info{};
    runtime_info.Initialize(stage);
    runtime_info.num_user_data = 0u;
    runtime_info.num_input_vgprs = 0u;
    runtime_info.num_allocated_vgprs = 0u;
    runtime_info.fp_denorm_mode32 = AmdGpu::FpDenormMode::InOutFlush;
    runtime_info.fp_denorm_mode16_64 = AmdGpu::FpDenormMode::InOutFlush;
    runtime_info.fp_round_mode32 = AmdGpu::FpRoundMode::NearestEven;
    runtime_info.fp_round_mode16_64 = AmdGpu::FpRoundMode::NearestEven;
    return runtime_info;
}

void PopulateUnsupportedResult(ElisaRecompilerProgramHandle& handle,
                               const std::uint64_t input_hash, const std::uint64_t word_count,
                               const std::uint32_t stage_type) {
    handle.input_hash = input_hash;
    handle.word_count = word_count;
    handle.stage_type = stage_type;
    handle.has_stage_type = 1u;
    handle.output_byte_size = 0u;
    handle.output_hash = 0u;
    handle.status = kElisaRecompilerProgramStatusFailedWithDiagnostic;
    handle.diagnostic_code = kElisaRecompilerDiagnosticUnsupported;
    handle.output_data.clear();
}

void PopulateTranslatedResult(ElisaRecompilerProgramHandle& handle,
                              const std::uint64_t input_hash, const std::uint64_t word_count,
                              const std::uint32_t stage_type,
                              const std::vector<std::uint8_t>& output_bytes) {
    handle.input_hash = input_hash;
    handle.word_count = word_count;
    handle.stage_type = stage_type;
    handle.has_stage_type = 1u;
    handle.output_data = output_bytes;
    handle.output_byte_size = static_cast<std::uint64_t>(handle.output_data.size());
    handle.output_hash = FnvHash64Bytes(handle.output_data.data(), handle.output_data.size());
    handle.status = kElisaRecompilerProgramStatusTranslated;
    handle.diagnostic_code = kElisaRecompilerDiagnosticNone;
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

namespace Shader {

IR::Program TranslateProgramForBridge(const std::span<const u32>& code, Pools& pools, Info& info,
                                      RuntimeInfo& runtime_info, const Profile& profile) {
    constexpr u32 token_mov_vcchi = 0xBEEB03FF;
    if (code[0] != token_mov_vcchi) {
        LOG_WARNING(Render_Recompiler, "First instruction is not s_mov_b32 vcc_hi, #imm");
    }

    Gcn::GcnCodeSlice slice(code.data(), code.data() + code.size());
    Gcn::GcnDecodeContext decoder;

    IR::Program program{info};
    program.ins_list.reserve(code.size());
    u32 decode_pc = 0;
    while (!slice.atEnd()) {
        const u32 token = slice.at(0);
        if (Gcn::GetInstructionEncoding(token) == Gcn::InstEncoding::ILLEGAL &&
            !program.ins_list.empty()) {
            LOG_WARNING(Render_Recompiler,
                        "Stopping shader decode at pc={:#x}: illegal encoding token {:#x} after "
                        "{} instructions. Treating remaining dwords as trailing metadata.",
                        decode_pc, token, program.ins_list.size());
            break;
        }
        auto inst = decoder.decodeInstruction(slice);
        if (inst.category == Gcn::InstCategory::Undefined && !program.ins_list.empty()) {
            LOG_WARNING(Render_Recompiler,
                        "Stopping shader decode at pc={:#x}: undefined opcode {} after {} "
                        "instructions. Treating remaining dwords as trailing metadata.",
                        decode_pc, static_cast<u32>(inst.opcode), program.ins_list.size());
            break;
        }
        decode_pc += inst.length;
        program.ins_list.emplace_back(inst);
    }

    pools.ReleaseContents();

    Common::ObjectPool<Gcn::Block> gcn_block_pool{64};
    Gcn::CFG cfg{gcn_block_pool, program.ins_list};

    program.syntax_list = Shader::Gcn::BuildASL(pools.inst_pool, pools.block_pool, cfg, info,
                                                runtime_info, profile);
    if (program.syntax_list.empty()) {
        return program;
    }
    program.blocks = GenerateBlocks(program.syntax_list);
    program.post_order_blocks = Shader::IR::PostOrder(program.syntax_list.front());

    Shader::InjectClipDistanceAttributes(program, runtime_info, profile);

    if (!profile.support_float64) {
        Shader::Optimization::LowerFp64ToFp32(program);
    }
    Shader::Optimization::SsaRewritePass(program.post_order_blocks);
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    Shader::Optimization::IdentityRemovalPass(program.blocks);
    if (info.l_stage == LogicalStage::TessellationControl) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::HullShaderTransform(program, runtime_info);
    } else if (info.l_stage == LogicalStage::TessellationEval) {
        Shader::Optimization::TessellationPreprocess(program, runtime_info);
        Shader::Optimization::DomainShaderTransform(program, runtime_info);
    }
    Shader::Optimization::RingAccessElimination(program, runtime_info);
    Shader::Optimization::ReadLaneEliminationPass(program);
    Shader::Optimization::ResourceTrackingPass(program, profile);
    Shader::Optimization::LowerBufferFormatToRaw(program);
    Shader::Optimization::SharedMemorySimplifyPass(program, profile);
    Shader::Optimization::SharedMemoryToStoragePass(program, runtime_info, profile);
    Shader::Optimization::SharedMemoryBarrierPass(program, runtime_info, profile);
    Shader::Optimization::IdentityRemovalPass(program.blocks);
    Shader::Optimization::DeadCodeEliminationPass(program);
    Shader::Optimization::ConstantPropagationPass(program.post_order_blocks);
    Shader::Optimization::CollectShaderInfoPass(program, profile);

    return program;
}

} // namespace Shader

extern "C" {

int elisa_shader_recompiler_translate_program(const std::uint32_t* code_words,
                                              std::size_t code_word_count,
                                              const ElisaRecompilerTranslateMetadata* metadata,
                                              void** out_program) {
    if (out_program) {
        *out_program = nullptr;
    }
    if (!code_words || code_word_count == 0 || !metadata || !out_program) {
        return kElisaRecompilerBridgeInvalidArgs;
    }
    auto* handle = new (std::nothrow) ElisaRecompilerProgramHandle{};
    if (!handle) {
        *out_program = nullptr;
        return kElisaRecompilerBridgeAllocFailed;
    }
    handle->magic = kElisaRecompilerProgramMagicLive;
    handle->stage_type = metadata->stage;
    handle->has_stage_type = 1u;
    const std::uint64_t input_hash = FnvHash64Words(code_words, code_word_count);
    const std::uint64_t word_count = static_cast<std::uint64_t>(code_word_count);
    PopulateUnsupportedResult(*handle, input_hash, word_count, metadata->stage);
    {
        std::lock_guard<std::mutex> lock{g_handle_mutex};
        g_freed_handles.erase(handle);
        g_live_handles.insert(handle);
    }
    Shader::Stage stage{};
    Shader::LogicalStage logical_stage{};
    if (!TryTranslateStage(metadata->stage, stage) ||
        !TryTranslateLogicalStage(metadata->logical_stage, logical_stage)) {
        *out_program = handle;
        return kElisaRecompilerBridgeOk;
    }

    try {
        constexpr std::size_t kNumUserData = Shader::ShaderParams::NumShaderUserData;
        std::array<std::uint32_t, kNumUserData> user_data{};
        std::span<const std::uint32_t, kNumUserData> user_data_span{user_data};
        const std::span<const std::uint32_t> code_span{code_words, code_word_count};
        Shader::ShaderParams params{user_data_span, code_span, input_hash};
        Shader::Info info{stage, logical_stage, params};
        Shader::RuntimeInfo runtime_info = DefaultRuntimeInfo(stage);
        Shader::Profile profile = DefaultProfile();
        Shader::Pools pools{};
        const auto ir_program =
            Shader::TranslateProgramForBridge(code_span, pools, info, runtime_info, profile);
        const auto output_bytes = SerializeTranslatedProgram(ir_program);

        if (output_bytes.empty()) {
            PopulateUnsupportedResult(*handle, input_hash, word_count, metadata->stage);
        } else {
            PopulateTranslatedResult(*handle, input_hash, word_count, metadata->stage,
                                     output_bytes);
        }
    } catch (...) {
        PopulateUnsupportedResult(*handle, input_hash, word_count, metadata->stage);
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
// kElisaRecompilerDiagnosticUnsupported (1): native bridge path unavailable or unsupported.
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
