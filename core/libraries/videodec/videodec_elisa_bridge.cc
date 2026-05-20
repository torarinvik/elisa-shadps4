// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec_elisa_bridge.hh"

#include "common/alignment.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/videodec/videodec_error.h"
#include "core/libraries/videodec/videodec_impl.h"
#include "core/libraries/videodec/videodec2_impl.h"

#include <cstring>
#include <type_traits>

namespace {
template <typename Native, typename Bridge>
Native* AsNative(Bridge* value) {
    static_assert(sizeof(Native) == sizeof(Bridge));
    static_assert(alignof(Native) == alignof(Bridge));
    return reinterpret_cast<Native*>(value);
}

template <typename Native, typename Bridge>
const Native* AsNative(const Bridge* value) {
    static_assert(sizeof(Native) == sizeof(Bridge));
    static_assert(alignof(Native) == alignof(Bridge));
    return reinterpret_cast<const Native*>(value);
}

constexpr uint64_t kFallbackMemorySize = 16ULL * 1024ULL * 1024ULL;

uint64_t ComputeFrameSizeBytes(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }
    const uint32_t aligned_width = Common::AlignUp<uint32_t>(static_cast<uint32_t>(width), 256);
    const uint32_t aligned_height = Common::AlignUp<uint32_t>(static_cast<uint32_t>(height), 16);
    return (static_cast<uint64_t>(aligned_width) * static_cast<uint64_t>(aligned_height) * 3) / 2;
}

int32_t ComputeDpbCount(const VideodecElisaConfigInfo& cfg) {
    return cfg.maxDpbFrameCount > 0 ? cfg.maxDpbFrameCount : 8;
}

void ComputeWorstCaseDimensions(const VideodecElisaConfigInfo& cfg, int32_t& width,
                                int32_t& height) {
    if (cfg.maxFrameWidth > 0 && cfg.maxFrameHeight > 0) {
        width = cfg.maxFrameWidth;
        height = cfg.maxFrameHeight;
        return;
    }
    width = cfg.maxLevel >= 150 ? 3840 : 1920;
    height = cfg.maxLevel >= 150 ? 2160 : 1080;
}
} // namespace

static_assert(sizeof(VideodecElisaConfigInfo) == sizeof(Libraries::Videodec::OrbisVideodecConfigInfo));
static_assert(sizeof(VideodecElisaResourceInfo) == sizeof(Libraries::Videodec::OrbisVideodecResourceInfo));
static_assert(sizeof(VideodecElisaCtrl) == sizeof(Libraries::Videodec::OrbisVideodecCtrl));
static_assert(sizeof(VideodecElisaFrameBuffer) == sizeof(Libraries::Videodec::OrbisVideodecFrameBuffer));
static_assert(sizeof(VideodecElisaPictureInfo) == sizeof(Libraries::Videodec::OrbisVideodecPictureInfo));
static_assert(sizeof(VideodecElisaInputData) == sizeof(Libraries::Videodec::OrbisVideodecInputData));

static_assert(sizeof(Videodec2ElisaDecoderConfigInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2DecoderConfigInfo));
static_assert(sizeof(Videodec2ElisaDecoderMemoryInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2DecoderMemoryInfo));
static_assert(sizeof(Videodec2ElisaInputData) == sizeof(Libraries::Videodec2::OrbisVideodec2InputData));
static_assert(sizeof(Videodec2ElisaOutputInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2OutputInfo));
static_assert(sizeof(Videodec2ElisaFrameBuffer) == sizeof(Libraries::Videodec2::OrbisVideodec2FrameBuffer));
static_assert(sizeof(Videodec2ElisaComputeMemoryInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2ComputeMemoryInfo));
static_assert(sizeof(Videodec2ElisaComputeConfigInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2ComputeConfigInfo));
static_assert(sizeof(Videodec2ElisaAvcPictureInfo) == sizeof(Libraries::Videodec2::OrbisVideodec2AvcPictureInfo));

extern "C" {
int VideodecElisa_sceVideodecCreateDecoder(const VideodecElisaConfigInfo* cfg,
                                            const VideodecElisaResourceInfo* rsrc,
                                            VideodecElisaCtrl* ctrl) {
    if (!cfg || !rsrc || !ctrl) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (cfg->thisSize != sizeof(VideodecElisaConfigInfo) ||
        rsrc->thisSize != sizeof(VideodecElisaResourceInfo)) {
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }
    auto* decoder = new Libraries::Videodec::VdecDecoder(
        *AsNative<Libraries::Videodec::OrbisVideodecConfigInfo>(cfg),
        *AsNative<Libraries::Videodec::OrbisVideodecResourceInfo>(rsrc));
    ctrl->thisSize = sizeof(VideodecElisaCtrl);
    ctrl->handle = decoder;
    ctrl->version = 1;
    return ORBIS_OK;
}

int VideodecElisa_sceVideodecDecode(VideodecElisaCtrl* ctrl,
                                     const VideodecElisaInputData* input,
                                     VideodecElisaFrameBuffer* frame,
                                     VideodecElisaPictureInfo* picture) {
    if (!ctrl || !input || !picture) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (ctrl->thisSize != sizeof(VideodecElisaCtrl) ||
        frame->thisSize != sizeof(VideodecElisaFrameBuffer)) {
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }
    auto* decoder = static_cast<Libraries::Videodec::VdecDecoder*>(ctrl->handle);
    if (!decoder) {
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    return decoder->Decode(*AsNative<Libraries::Videodec::OrbisVideodecInputData>(input),
                           *AsNative<Libraries::Videodec::OrbisVideodecFrameBuffer>(frame),
                           *AsNative<Libraries::Videodec::OrbisVideodecPictureInfo>(picture));
}

int VideodecElisa_sceVideodecDeleteDecoder(VideodecElisaCtrl* ctrl) {
    auto* decoder = static_cast<Libraries::Videodec::VdecDecoder*>(ctrl->handle);
    if (!decoder) {
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    delete decoder;
    ctrl->handle = nullptr;
    return ORBIS_OK;
}

int VideodecElisa_sceVideodecFlush(VideodecElisaCtrl* ctrl,
                                    VideodecElisaFrameBuffer* frame,
                                    VideodecElisaPictureInfo* picture) {
    if (!frame || !picture) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (frame->thisSize != sizeof(VideodecElisaFrameBuffer) ||
        picture->thisSize != sizeof(VideodecElisaPictureInfo)) {
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }
    auto* decoder = static_cast<Libraries::Videodec::VdecDecoder*>(ctrl->handle);
    if (!decoder) {
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    return decoder->Flush(*AsNative<Libraries::Videodec::OrbisVideodecFrameBuffer>(frame),
                          *AsNative<Libraries::Videodec::OrbisVideodecPictureInfo>(picture));
}

int VideodecElisa_sceVideodecMapMemory(void) { return ORBIS_OK; }

int VideodecElisa_sceVideodecQueryResourceInfo(const VideodecElisaConfigInfo* cfg,
                                                VideodecElisaResourceInfo* rsrc) {
    if (!cfg || !rsrc) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (cfg->thisSize != sizeof(VideodecElisaConfigInfo) ||
        rsrc->thisSize != sizeof(VideodecElisaResourceInfo)) {
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }
    int32_t width = 0;
    int32_t height = 0;
    ComputeWorstCaseDimensions(*cfg, width, height);
    const uint64_t frame_size = ComputeFrameSizeBytes(width, height);
    uint64_t cpu_gpu_size = kFallbackMemorySize;
    uint64_t cpu_size = kFallbackMemorySize;
    uint64_t max_frame_buffer = kFallbackMemorySize;
    if (frame_size != 0) {
        const uint64_t padded_frame = Common::AlignUp<uint64_t>(frame_size, 256) + 0x4000;
        max_frame_buffer = padded_frame;
        cpu_gpu_size = (padded_frame * (static_cast<uint64_t>(ComputeDpbCount(*cfg)) + 2)) +
                       (8ULL * 1024ULL * 1024ULL);
        cpu_size = kFallbackMemorySize;
    }
    rsrc->thisSize = sizeof(VideodecElisaResourceInfo);
    rsrc->pCpuMemory = nullptr;
    rsrc->pCpuGpuMemory = nullptr;
    rsrc->cpuGpuMemorySize = cpu_gpu_size;
    rsrc->cpuMemorySize = cpu_size;
    rsrc->maxFrameBufferSize = max_frame_buffer;
    rsrc->frameBufferAlignment = 0x100;
    return ORBIS_OK;
}

int VideodecElisa_sceVideodecReset(VideodecElisaCtrl* ctrl) {
    auto* decoder = static_cast<Libraries::Videodec::VdecDecoder*>(ctrl->handle);
    return decoder->Reset();
}

void VideodecElisa_RegisterLib(void*) {}

int Videodec2Elisa_sceVideodec2QueryComputeMemoryInfo(Videodec2ElisaComputeMemoryInfo* info) {
    if (!info) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (info->thisSize != sizeof(Videodec2ElisaComputeMemoryInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    info->cpuGpuMemory = nullptr;
    info->cpuGpuMemorySize = kFallbackMemorySize;
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2AllocateComputeQueue(const Videodec2ElisaComputeConfigInfo* cfg,
                                                     const Videodec2ElisaComputeMemoryInfo* mem,
                                                     void** queue) {
    if (!cfg || !mem || !queue) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (cfg->thisSize != sizeof(Videodec2ElisaComputeConfigInfo) ||
        mem->thisSize != sizeof(Videodec2ElisaComputeMemoryInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (cfg->reserved0 != 0 || cfg->reserved1 != 0) {
        return ORBIS_VIDEODEC2_ERROR_CONFIG_INFO;
    }
    if (cfg->computePipeId > 4) {
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_PIPE_ID;
    }
    if (cfg->computeQueueId > 7) {
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_QUEUE_ID;
    }
    if (!mem->cpuGpuMemory) {
        return ORBIS_VIDEODEC2_ERROR_MEMORY_POINTER;
    }
    *queue = mem->cpuGpuMemory;
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2ReleaseComputeQueue(void*) { return ORBIS_OK; }

int Videodec2Elisa_sceVideodec2QueryDecoderMemoryInfo(const Videodec2ElisaDecoderConfigInfo* cfg,
                                                       Videodec2ElisaDecoderMemoryInfo* mem) {
    if (!cfg || !mem) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (cfg->thisSize != sizeof(Videodec2ElisaDecoderConfigInfo) ||
        mem->thisSize != sizeof(Videodec2ElisaDecoderMemoryInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    mem->cpuMemory = nullptr;
    mem->gpuMemory = nullptr;
    mem->cpuGpuMemory = nullptr;
    mem->cpuGpuMemorySize = kFallbackMemorySize;
    mem->cpuMemorySize = kFallbackMemorySize;
    mem->gpuMemorySize = kFallbackMemorySize;
    mem->maxFrameBufferSize = kFallbackMemorySize;
    mem->frameBufferAlignment = 0x100;
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2CreateDecoder(const Videodec2ElisaDecoderConfigInfo* cfg,
                                              const Videodec2ElisaDecoderMemoryInfo* mem,
                                              void** decoder) {
    if (!cfg || !mem || !decoder) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (cfg->thisSize != sizeof(Videodec2ElisaDecoderConfigInfo) ||
        mem->thisSize != sizeof(Videodec2ElisaDecoderMemoryInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    *decoder = new Libraries::Videodec2::VdecDecoder(
        *AsNative<Libraries::Videodec2::OrbisVideodec2DecoderConfigInfo>(cfg),
        *AsNative<Libraries::Videodec2::OrbisVideodec2DecoderMemoryInfo>(mem));
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2DeleteDecoder(void* decoder) {
    if (!decoder) {
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    delete static_cast<Libraries::Videodec2::VdecDecoder*>(decoder);
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2Decode(void* decoder,
                                       const Videodec2ElisaInputData* input,
                                       Videodec2ElisaFrameBuffer* frame,
                                       Videodec2ElisaOutputInfo* output) {
    if (!decoder) {
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!input || !frame || !output) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (input->thisSize != sizeof(Videodec2ElisaInputData) ||
        frame->thisSize != sizeof(Videodec2ElisaFrameBuffer)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    return static_cast<Libraries::Videodec2::VdecDecoder*>(decoder)->Decode(
        *AsNative<Libraries::Videodec2::OrbisVideodec2InputData>(input),
        *AsNative<Libraries::Videodec2::OrbisVideodec2FrameBuffer>(frame),
        *AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output));
}

int Videodec2Elisa_sceVideodec2Flush(void* decoder,
                                      Videodec2ElisaFrameBuffer* frame,
                                      Videodec2ElisaOutputInfo* output) {
    if (!decoder) {
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!frame || !output) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (frame->thisSize != sizeof(Videodec2ElisaFrameBuffer) ||
        (output->thisSize | 8) != sizeof(Videodec2ElisaOutputInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    return static_cast<Libraries::Videodec2::VdecDecoder*>(decoder)->Flush(
        *AsNative<Libraries::Videodec2::OrbisVideodec2FrameBuffer>(frame),
        *AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output));
}

int Videodec2Elisa_sceVideodec2Reset(void* decoder) {
    if (!decoder) {
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    return static_cast<Libraries::Videodec2::VdecDecoder*>(decoder)->Reset();
}

int Videodec2Elisa_sceVideodec2GetPictureInfo(const Videodec2ElisaOutputInfo* output,
                                               void* first_picture,
                                               void*) {
    if (!output) {
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if ((output->thisSize | 8) != sizeof(Videodec2ElisaOutputInfo)) {
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (output->pictureCount == 0 || Libraries::Videodec2::gPictureInfos.empty()) {
        return ORBIS_OK;
    }
    if (first_picture) {
        uint64_t picture_size = 0;
        std::memcpy(&picture_size, first_picture, sizeof(uint64_t));
        if ((picture_size | 0x10) != sizeof(Libraries::Videodec2::OrbisVideodec2AvcPictureInfo)) {
            return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
        }
        std::memcpy(first_picture, &Libraries::Videodec2::gPictureInfos.back(), picture_size);
        std::memcpy(first_picture, &picture_size, sizeof(uint64_t));
    }
    return ORBIS_OK;
}

int Videodec2Elisa_sceVideodec2GetAvcPictureInfo(const Videodec2ElisaOutputInfo* output,
                                                  void* first_picture,
                                                  void* second_picture) {
    return Videodec2Elisa_sceVideodec2GetPictureInfo(output, first_picture, second_picture);
}

void Videodec2Elisa_RegisterLib(void*) {}
} // extern "C"
