// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec_elisa_bridge.hh"

#include <stddef.h>
#include <string.h>

#define ORBIS_OK 0
#define VIDEODEC_16_MB 16777216ULL
#define ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER ((int)0x80C1000F)
#define ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER ((int)0x811D0102)

int VideodecElisa_sceVideodecCreateDecoder(const struct VideodecElisaConfigInfo* cfg,
                                            const struct VideodecElisaResourceInfo* rsrc,
                                            struct VideodecElisaCtrl* ctrl) {
    if (!cfg || !rsrc || !ctrl) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    ctrl->thisSize = sizeof(*ctrl);
    ctrl->handle = (void*)0x51515151ULL;
    ctrl->version = 1;
    return ORBIS_OK;
}

int VideodecElisa_sceVideodecDecode(struct VideodecElisaCtrl* ctrl,
                                     const struct VideodecElisaInputData* input,
                                     struct VideodecElisaFrameBuffer* frame,
                                     struct VideodecElisaPictureInfo* picture) {
    if (!ctrl || !input || !frame || !picture) {
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    picture->thisSize = sizeof(*picture);
    picture->isValid = 1;
    picture->isErrorPic = 0;
    picture->ptsData = input->ptsData;
    picture->attachedData = input->attachedData;
    return ORBIS_OK;
}

int VideodecElisa_sceVideodecDeleteDecoder(struct VideodecElisaCtrl* ctrl) { return ctrl ? ORBIS_OK : ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER; }
int VideodecElisa_sceVideodecFlush(struct VideodecElisaCtrl* ctrl, struct VideodecElisaFrameBuffer* frame, struct VideodecElisaPictureInfo* picture) { return (ctrl && frame && picture) ? ORBIS_OK : ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER; }
int VideodecElisa_sceVideodecMapMemory(void) { return ORBIS_OK; }
int VideodecElisa_sceVideodecQueryResourceInfo(const struct VideodecElisaConfigInfo* cfg, struct VideodecElisaResourceInfo* rsrc) {
    if (!cfg || !rsrc) { return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER; }
    rsrc->thisSize = sizeof(*rsrc);
    rsrc->cpuMemorySize = VIDEODEC_16_MB;
    rsrc->cpuGpuMemorySize = VIDEODEC_16_MB;
    rsrc->maxFrameBufferSize = VIDEODEC_16_MB;
    rsrc->frameBufferAlignment = 0x100;
    return ORBIS_OK;
}
int VideodecElisa_sceVideodecReset(struct VideodecElisaCtrl* ctrl) { return ctrl ? ORBIS_OK : ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER; }
void VideodecElisa_RegisterLib(void* sym) { (void)sym; }

int Videodec2Elisa_sceVideodec2QueryComputeMemoryInfo(struct Videodec2ElisaComputeMemoryInfo* info) {
    if (!info) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
    info->cpuGpuMemorySize = VIDEODEC_16_MB;
    info->cpuGpuMemory = NULL;
    return ORBIS_OK;
}
int Videodec2Elisa_sceVideodec2AllocateComputeQueue(const struct Videodec2ElisaComputeConfigInfo* cfg, const struct Videodec2ElisaComputeMemoryInfo* mem, void** queue) {
    if (!cfg || !mem || !queue) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
    *queue = mem->cpuGpuMemory;
    return ORBIS_OK;
}
int Videodec2Elisa_sceVideodec2ReleaseComputeQueue(void* queue) { (void)queue; return ORBIS_OK; }
int Videodec2Elisa_sceVideodec2QueryDecoderMemoryInfo(const struct Videodec2ElisaDecoderConfigInfo* cfg, struct Videodec2ElisaDecoderMemoryInfo* mem) {
    if (!cfg || !mem) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
    mem->cpuMemorySize = VIDEODEC_16_MB;
    mem->gpuMemorySize = VIDEODEC_16_MB;
    mem->cpuGpuMemorySize = VIDEODEC_16_MB;
    mem->maxFrameBufferSize = VIDEODEC_16_MB;
    mem->frameBufferAlignment = 0x100;
    return ORBIS_OK;
}
int Videodec2Elisa_sceVideodec2CreateDecoder(const struct Videodec2ElisaDecoderConfigInfo* cfg, const struct Videodec2ElisaDecoderMemoryInfo* mem, void** decoder) {
    if (!cfg || !mem || !decoder) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
    *decoder = (void*)0x52525252ULL;
    return ORBIS_OK;
}
int Videodec2Elisa_sceVideodec2DeleteDecoder(void* decoder) { return decoder ? ORBIS_OK : ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
int Videodec2Elisa_sceVideodec2Decode(void* decoder, const struct Videodec2ElisaInputData* input, struct Videodec2ElisaFrameBuffer* frame, struct Videodec2ElisaOutputInfo* output) {
    if (!decoder || !input || !frame || !output) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
    frame->isAccepted = true;
    output->isValid = true;
    output->isErrorFrame = false;
    output->pictureCount = 1;
    output->frameBuffer = frame->frameBuffer;
    output->frameBufferSize = frame->frameBufferSize;
    return ORBIS_OK;
}
int Videodec2Elisa_sceVideodec2Flush(void* decoder, struct Videodec2ElisaFrameBuffer* frame, struct Videodec2ElisaOutputInfo* output) { return (decoder && frame && output) ? ORBIS_OK : ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
int Videodec2Elisa_sceVideodec2Reset(void* decoder) { return decoder ? ORBIS_OK : ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; }
int Videodec2Elisa_sceVideodec2GetPictureInfo(const struct Videodec2ElisaOutputInfo* output, void* first_picture, void* second_picture) { (void)second_picture; if (!output) { return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER; } if (first_picture) { memset(first_picture, 0, sizeof(struct Videodec2ElisaAvcPictureInfo)); } return ORBIS_OK; }
int Videodec2Elisa_sceVideodec2GetAvcPictureInfo(const struct Videodec2ElisaOutputInfo* output, void* first_picture, void* second_picture) { return Videodec2Elisa_sceVideodec2GetPictureInfo(output, first_picture, second_picture); }
void Videodec2Elisa_RegisterLib(void* sym) { (void)sym; }
