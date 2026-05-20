// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/videodec/videodec_elisa_bridge.hh"

#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec2.h"

#include <cstddef>
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
    return Libraries::Videodec::sceVideodecCreateDecoder(
        AsNative<Libraries::Videodec::OrbisVideodecConfigInfo>(cfg),
        AsNative<Libraries::Videodec::OrbisVideodecResourceInfo>(rsrc),
        AsNative<Libraries::Videodec::OrbisVideodecCtrl>(ctrl));
}

int VideodecElisa_sceVideodecDecode(VideodecElisaCtrl* ctrl,
                                     const VideodecElisaInputData* input,
                                     VideodecElisaFrameBuffer* frame,
                                     VideodecElisaPictureInfo* picture) {
    return Libraries::Videodec::sceVideodecDecode(
        AsNative<Libraries::Videodec::OrbisVideodecCtrl>(ctrl),
        AsNative<Libraries::Videodec::OrbisVideodecInputData>(input),
        AsNative<Libraries::Videodec::OrbisVideodecFrameBuffer>(frame),
        AsNative<Libraries::Videodec::OrbisVideodecPictureInfo>(picture));
}

int VideodecElisa_sceVideodecDeleteDecoder(VideodecElisaCtrl* ctrl) {
    return Libraries::Videodec::sceVideodecDeleteDecoder(
        AsNative<Libraries::Videodec::OrbisVideodecCtrl>(ctrl));
}

int VideodecElisa_sceVideodecFlush(VideodecElisaCtrl* ctrl,
                                    VideodecElisaFrameBuffer* frame,
                                    VideodecElisaPictureInfo* picture) {
    return Libraries::Videodec::sceVideodecFlush(
        AsNative<Libraries::Videodec::OrbisVideodecCtrl>(ctrl),
        AsNative<Libraries::Videodec::OrbisVideodecFrameBuffer>(frame),
        AsNative<Libraries::Videodec::OrbisVideodecPictureInfo>(picture));
}

int VideodecElisa_sceVideodecMapMemory(void) {
    return Libraries::Videodec::sceVideodecMapMemory();
}

int VideodecElisa_sceVideodecQueryResourceInfo(const VideodecElisaConfigInfo* cfg,
                                                VideodecElisaResourceInfo* rsrc) {
    return Libraries::Videodec::sceVideodecQueryResourceInfo(
        AsNative<Libraries::Videodec::OrbisVideodecConfigInfo>(cfg),
        AsNative<Libraries::Videodec::OrbisVideodecResourceInfo>(rsrc));
}

int VideodecElisa_sceVideodecReset(VideodecElisaCtrl* ctrl) {
    return Libraries::Videodec::sceVideodecReset(
        AsNative<Libraries::Videodec::OrbisVideodecCtrl>(ctrl));
}

void VideodecElisa_RegisterLib(void* sym) {
    Libraries::Videodec::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}

int Videodec2Elisa_sceVideodec2QueryComputeMemoryInfo(Videodec2ElisaComputeMemoryInfo* info) {
    return Libraries::Videodec2::sceVideodec2QueryComputeMemoryInfo(
        AsNative<Libraries::Videodec2::OrbisVideodec2ComputeMemoryInfo>(info));
}

int Videodec2Elisa_sceVideodec2AllocateComputeQueue(const Videodec2ElisaComputeConfigInfo* cfg,
                                                     const Videodec2ElisaComputeMemoryInfo* mem,
                                                     void** queue) {
    return Libraries::Videodec2::sceVideodec2AllocateComputeQueue(
        AsNative<Libraries::Videodec2::OrbisVideodec2ComputeConfigInfo>(cfg),
        AsNative<Libraries::Videodec2::OrbisVideodec2ComputeMemoryInfo>(mem), queue);
}

int Videodec2Elisa_sceVideodec2ReleaseComputeQueue(void* queue) {
    return Libraries::Videodec2::sceVideodec2ReleaseComputeQueue(queue);
}

int Videodec2Elisa_sceVideodec2QueryDecoderMemoryInfo(const Videodec2ElisaDecoderConfigInfo* cfg,
                                                       Videodec2ElisaDecoderMemoryInfo* mem) {
    return Libraries::Videodec2::sceVideodec2QueryDecoderMemoryInfo(
        AsNative<Libraries::Videodec2::OrbisVideodec2DecoderConfigInfo>(cfg),
        AsNative<Libraries::Videodec2::OrbisVideodec2DecoderMemoryInfo>(mem));
}

int Videodec2Elisa_sceVideodec2CreateDecoder(const Videodec2ElisaDecoderConfigInfo* cfg,
                                              const Videodec2ElisaDecoderMemoryInfo* mem,
                                              void** decoder) {
    auto native_decoder = reinterpret_cast<Libraries::Videodec2::OrbisVideodec2Decoder*>(decoder);
    return Libraries::Videodec2::sceVideodec2CreateDecoder(
        AsNative<Libraries::Videodec2::OrbisVideodec2DecoderConfigInfo>(cfg),
        AsNative<Libraries::Videodec2::OrbisVideodec2DecoderMemoryInfo>(mem), native_decoder);
}

int Videodec2Elisa_sceVideodec2DeleteDecoder(void* decoder) {
    return Libraries::Videodec2::sceVideodec2DeleteDecoder(
        static_cast<Libraries::Videodec2::OrbisVideodec2Decoder>(decoder));
}

int Videodec2Elisa_sceVideodec2Decode(void* decoder,
                                       const Videodec2ElisaInputData* input,
                                       Videodec2ElisaFrameBuffer* frame,
                                       Videodec2ElisaOutputInfo* output) {
    return Libraries::Videodec2::sceVideodec2Decode(
        static_cast<Libraries::Videodec2::OrbisVideodec2Decoder>(decoder),
        AsNative<Libraries::Videodec2::OrbisVideodec2InputData>(input),
        AsNative<Libraries::Videodec2::OrbisVideodec2FrameBuffer>(frame),
        AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output));
}

int Videodec2Elisa_sceVideodec2Flush(void* decoder,
                                      Videodec2ElisaFrameBuffer* frame,
                                      Videodec2ElisaOutputInfo* output) {
    return Libraries::Videodec2::sceVideodec2Flush(
        static_cast<Libraries::Videodec2::OrbisVideodec2Decoder>(decoder),
        AsNative<Libraries::Videodec2::OrbisVideodec2FrameBuffer>(frame),
        AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output));
}

int Videodec2Elisa_sceVideodec2Reset(void* decoder) {
    return Libraries::Videodec2::sceVideodec2Reset(
        static_cast<Libraries::Videodec2::OrbisVideodec2Decoder>(decoder));
}

int Videodec2Elisa_sceVideodec2GetPictureInfo(const Videodec2ElisaOutputInfo* output,
                                               void* first_picture,
                                               void* second_picture) {
    return Libraries::Videodec2::sceVideodec2GetPictureInfo(
        AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output), first_picture,
        second_picture);
}

int Videodec2Elisa_sceVideodec2GetAvcPictureInfo(const Videodec2ElisaOutputInfo* output,
                                                  void* first_picture,
                                                  void* second_picture) {
    return Libraries::Videodec2::sceVideodec2GetAvcPictureInfo(
        AsNative<Libraries::Videodec2::OrbisVideodec2OutputInfo>(output), first_picture,
        second_picture);
}

void Videodec2Elisa_RegisterLib(void* sym) {
    Libraries::Videodec2::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}
} // extern "C"
