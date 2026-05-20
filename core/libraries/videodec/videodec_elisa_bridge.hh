// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VideodecElisaConfigInfo {
    uint64_t thisSize;
    uint32_t codecType;
    uint32_t profile;
    uint32_t maxLevel;
    int32_t maxFrameWidth;
    int32_t maxFrameHeight;
    int32_t maxDpbFrameCount;
    uint64_t videodecFlags;
};

struct VideodecElisaResourceInfo {
    uint64_t thisSize;
    uint64_t cpuMemorySize;
    void* pCpuMemory;
    uint64_t cpuGpuMemorySize;
    void* pCpuGpuMemory;
    uint64_t maxFrameBufferSize;
    uint32_t frameBufferAlignment;
};

struct VideodecElisaCtrl {
    uint64_t thisSize;
    void* handle;
    uint64_t version;
};

struct VideodecElisaFrameBuffer {
    uint64_t thisSize;
    void* pFrameBuffer;
    uint64_t frameBufferSize;
};

struct VideodecElisaAvcInfo {
    uint32_t numUnitsInTick;
    uint32_t timeScale;
    uint8_t fixedFrameRateFlag;
    uint8_t aspectRatioIdc;
    uint16_t sarWidth;
    uint16_t sarHeight;
    uint8_t colourPrimaries;
    uint8_t transferCharacteristics;
    uint8_t matrixCoefficients;
    uint8_t videoFullRangeFlag;
    uint32_t frameCropLeftOffset;
    uint32_t frameCropRightOffset;
    uint32_t frameCropTopOffset;
    uint32_t frameCropBottomOffset;
};

struct VideodecElisaPictureInfo {
    uint64_t thisSize;
    uint32_t isValid;
    uint32_t codecType;
    uint32_t frameWidth;
    uint32_t framePitch;
    uint32_t frameHeight;
    uint32_t isErrorPic;
    uint64_t ptsData;
    uint64_t attachedData;
    uint8_t codec[64];
};

struct VideodecElisaInputData {
    uint64_t thisSize;
    void* pAuData;
    uint64_t auSize;
    uint64_t ptsData;
    uint64_t dtsData;
    uint64_t attachedData;
};

struct Videodec2ElisaDecoderConfigInfo {
    uint64_t thisSize;
    uint32_t resourceType;
    uint32_t codecType;
    uint32_t profile;
    uint32_t maxLevel;
    int32_t maxFrameWidth;
    int32_t maxFrameHeight;
    int32_t maxDpbFrameCount;
    uint32_t decodePipelineDepth;
    void* computeQueue;
    uint64_t cpuAffinityMask;
    int32_t cpuThreadPriority;
    bool optimizeProgressiveVideo;
    bool checkMemoryType;
    uint8_t reserved0;
    uint8_t reserved1;
    void* extraConfigInfo;
};

struct Videodec2ElisaDecoderMemoryInfo {
    uint64_t thisSize;
    uint64_t cpuMemorySize;
    void* cpuMemory;
    uint64_t gpuMemorySize;
    void* gpuMemory;
    uint64_t cpuGpuMemorySize;
    void* cpuGpuMemory;
    uint64_t maxFrameBufferSize;
    uint32_t frameBufferAlignment;
    uint32_t reserved0;
};

struct Videodec2ElisaInputData {
    uint64_t thisSize;
    void* auData;
    uint64_t auSize;
    uint64_t ptsData;
    uint64_t dtsData;
    uint64_t attachedData;
};

struct Videodec2ElisaOutputInfo {
    uint64_t thisSize;
    bool isValid;
    bool isErrorFrame;
    uint8_t pictureCount;
    uint32_t codecType;
    uint32_t frameWidth;
    uint32_t framePitch;
    uint32_t frameHeight;
    void* frameBuffer;
    uint64_t frameBufferSize;
    uint32_t frameFormat;
    uint32_t framePitchInBytes;
};

struct Videodec2ElisaFrameBuffer {
    uint64_t thisSize;
    void* frameBuffer;
    uint64_t frameBufferSize;
    bool isAccepted;
};

struct Videodec2ElisaComputeMemoryInfo {
    uint64_t thisSize;
    uint64_t cpuGpuMemorySize;
    void* cpuGpuMemory;
};

struct Videodec2ElisaComputeConfigInfo {
    uint64_t thisSize;
    uint16_t computePipeId;
    uint16_t computeQueueId;
    bool checkMemoryType;
    uint8_t reserved0;
    uint16_t reserved1;
};

struct Videodec2ElisaAvcPictureInfo {
    uint64_t thisSize;
    bool isValid;
    uint64_t ptsData;
    uint64_t dtsData;
    uint64_t attachedData;
    uint8_t idrPictureflag;
    uint8_t profile_idc;
    uint8_t level_idc;
    uint32_t pic_width_in_mbs_minus1;
    uint32_t pic_height_in_map_units_minus1;
    uint8_t frame_mbs_only_flag;
    uint8_t frame_cropping_flag;
    uint32_t frameCropLeftOffset;
    uint32_t frameCropRightOffset;
    uint32_t frameCropTopOffset;
    uint32_t frameCropBottomOffset;
    uint8_t aspect_ratio_info_present_flag;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    uint8_t video_signal_type_present_flag;
    uint8_t video_format;
    uint8_t video_full_range_flag;
    uint8_t colour_description_present_flag;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    uint8_t timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    uint8_t fixed_frame_rate_flag;
    uint8_t bitstream_restriction_flag;
    uint8_t max_dec_frame_buffering;
    uint8_t pic_struct_present_flag;
    uint8_t pic_struct;
    uint8_t field_pic_flag;
    uint8_t bottom_field_flag;
    uint8_t sequenceParameterSetPresentFlag;
    uint8_t pictureParameterSetPresentFlag;
    uint8_t auDelimiterPresentFlag;
    uint8_t endOfSequencePresentFlag;
    uint8_t endOfStreamPresentFlag;
    uint8_t fillerDataPresentFlag;
    uint8_t pictureTimingSeiPresentFlag;
    uint8_t bufferingPeriodSeiPresentFlag;
    uint8_t constraint_set0_flag;
    uint8_t constraint_set1_flag;
    uint8_t constraint_set2_flag;
    uint8_t constraint_set3_flag;
    uint8_t constraint_set4_flag;
    uint8_t constraint_set5_flag;
};

int VideodecElisa_sceVideodecCreateDecoder(const struct VideodecElisaConfigInfo* cfg,
                                            const struct VideodecElisaResourceInfo* rsrc,
                                            struct VideodecElisaCtrl* ctrl);
int VideodecElisa_sceVideodecDecode(struct VideodecElisaCtrl* ctrl,
                                     const struct VideodecElisaInputData* input,
                                     struct VideodecElisaFrameBuffer* frame,
                                     struct VideodecElisaPictureInfo* picture);
int VideodecElisa_sceVideodecDeleteDecoder(struct VideodecElisaCtrl* ctrl);
int VideodecElisa_sceVideodecFlush(struct VideodecElisaCtrl* ctrl,
                                    struct VideodecElisaFrameBuffer* frame,
                                    struct VideodecElisaPictureInfo* picture);
int VideodecElisa_sceVideodecMapMemory(void);
int VideodecElisa_sceVideodecQueryResourceInfo(const struct VideodecElisaConfigInfo* cfg,
                                                struct VideodecElisaResourceInfo* rsrc);
int VideodecElisa_sceVideodecReset(struct VideodecElisaCtrl* ctrl);
void VideodecElisa_RegisterLib(void* sym);

int Videodec2Elisa_sceVideodec2QueryComputeMemoryInfo(
    struct Videodec2ElisaComputeMemoryInfo* info);
int Videodec2Elisa_sceVideodec2AllocateComputeQueue(
    const struct Videodec2ElisaComputeConfigInfo* cfg,
    const struct Videodec2ElisaComputeMemoryInfo* mem,
    void** queue);
int Videodec2Elisa_sceVideodec2ReleaseComputeQueue(void* queue);
int Videodec2Elisa_sceVideodec2QueryDecoderMemoryInfo(
    const struct Videodec2ElisaDecoderConfigInfo* cfg,
    struct Videodec2ElisaDecoderMemoryInfo* mem);
int Videodec2Elisa_sceVideodec2CreateDecoder(
    const struct Videodec2ElisaDecoderConfigInfo* cfg,
    const struct Videodec2ElisaDecoderMemoryInfo* mem,
    void** decoder);
int Videodec2Elisa_sceVideodec2DeleteDecoder(void* decoder);
int Videodec2Elisa_sceVideodec2Decode(void* decoder,
                                       const struct Videodec2ElisaInputData* input,
                                       struct Videodec2ElisaFrameBuffer* frame,
                                       struct Videodec2ElisaOutputInfo* output);
int Videodec2Elisa_sceVideodec2Flush(void* decoder,
                                      struct Videodec2ElisaFrameBuffer* frame,
                                      struct Videodec2ElisaOutputInfo* output);
int Videodec2Elisa_sceVideodec2Reset(void* decoder);
int Videodec2Elisa_sceVideodec2GetPictureInfo(const struct Videodec2ElisaOutputInfo* output,
                                               void* first_picture,
                                               void* second_picture);
int Videodec2Elisa_sceVideodec2GetAvcPictureInfo(const struct Videodec2ElisaOutputInfo* output,
                                                  void* first_picture,
                                                  void* second_picture);
void Videodec2Elisa_RegisterLib(void* sym);

#ifdef __cplusplus
} // extern "C"
#endif
