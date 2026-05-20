// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/videoout/video_out.h"

namespace Libraries::VideoOut {

struct Mode {
    u32 size;
    u8 encoding;
    u8 range;
    u8 colorimetry;
    u8 depth;
    u64 refresh_rate;
    u64 resolution;
    u8 reserved[8];
};

s32 PS4_SYSV_ABI sceVideoOutDeleteFlipEvent(Kernel::OrbisKernelEqueue eq, s32 handle);
s32 PS4_SYSV_ABI sceVideoOutDeleteVblankEvent(Kernel::OrbisKernelEqueue eq, s32 handle);
s32 PS4_SYSV_ABI sceVideoOutGetVblankStatus(int handle, SceVideoOutVblankStatus* status);
s32 PS4_SYSV_ABI sceVideoOutGetDeviceCapabilityInfo(
    s32 handle, SceVideoOutDeviceCapabilityInfo* pDeviceCapabilityInfo);
s32 PS4_SYSV_ABI sceVideoOutGetEventCount(const Kernel::OrbisKernelEvent* ev);
s32 PS4_SYSV_ABI sceVideoOutUnregisterBuffers(s32 handle, s32 attributeIndex);
void PS4_SYSV_ABI sceVideoOutModeSetAny_(Mode* mode, u32 size);
s32 PS4_SYSV_ABI sceVideoOutConfigureOutputMode_(s32 handle, u32 reserved, const Mode* mode,
                                                 const void* options, u32 size_mode,
                                                 u32 size_options);
s32 PS4_SYSV_ABI sceVideoOutSubmitChangeBufferAttribute(s32 handle, s32 attributeIndex,
                                                        const BufferAttribute* attribute);
s32 PS4_SYSV_ABI sceVideoOutSetWindowModeMargins(s32 handle, s32 top, s32 bottom);

} // namespace Libraries::VideoOut

extern "C" {

int VideoOutElisa_sceVideoOutGetFlipStatus(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetFlipStatus(static_cast<int>(a0), static_cast<Libraries::VideoOut::FlipStatus*>(a1)));
}

int VideoOutElisa_sceVideoOutSubmitFlip(long long a0, long long a1, long long a2, long long a3) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutSubmitFlip(static_cast<int>(a0), static_cast<int>(a1), static_cast<int>(a2), static_cast<int>(a3)));
}

int VideoOutElisa_sceVideoOutRegisterBuffers(long long a0, long long a1, void* a2, long long a3, void* a4) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutRegisterBuffers(static_cast<int>(a0), static_cast<int>(a1), static_cast<void* const*>(a2), static_cast<int>(a3), static_cast<const Libraries::VideoOut::BufferAttribute*>(a4)));
}

int VideoOutElisa_sceVideoOutAddFlipEvent(long long a0, long long a1, void* a2) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutAddFlipEvent(static_cast<int>(a0), static_cast<int>(a1), static_cast<void*>(a2)));
}

int VideoOutElisa_sceVideoOutAddVblankEvent(long long a0, long long a1, void* a2) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutAddVblankEvent(static_cast<int>(a0), static_cast<int>(a1), static_cast<void*>(a2)));
}

int VideoOutElisa_sceVideoOutSetFlipRate(long long a0, long long a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutSetFlipRate(static_cast<int>(a0), static_cast<int>(a1)));
}

int VideoOutElisa_sceVideoOutSetBufferAttribute(void* a0, long long a1, long long a2, long long a3, long long a4, long long a5, long long a6) {
    Libraries::VideoOut::sceVideoOutSetBufferAttribute(static_cast<Libraries::VideoOut::BufferAttribute*>(a0), static_cast<Libraries::VideoOut::PixelFormat>(a1), static_cast<int>(a2), static_cast<int>(a3), static_cast<int>(a4), static_cast<int>(a5), static_cast<int>(a6));
    return 0;
}

int VideoOutElisa_sceVideoOutGetResolutionStatus(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetResolutionStatus(static_cast<int>(a0), static_cast<Libraries::VideoOut::SceVideoOutResolutionStatus*>(a1)));
}

int VideoOutElisa_sceVideoOutOpen(long long a0, long long a1, long long a2, void* a3) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutOpen(static_cast<Libraries::UserService::OrbisUserServiceUserId>(a0), static_cast<int>(a1), static_cast<int>(a2), static_cast<const void*>(a3)));
}

int VideoOutElisa_sceVideoOutIsFlipPending(long long a0) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutIsFlipPending(static_cast<int>(a0)));
}

int VideoOutElisa_sceVideoOutUnregisterBuffers(long long a0, long long a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutUnregisterBuffers(static_cast<int>(a0), static_cast<int>(a1)));
}

int VideoOutElisa_sceVideoOutGetBufferLabelAddress(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetBufferLabelAddress(static_cast<int>(a0), static_cast<uintptr_t*>(a1)));
}

int VideoOutElisa_sceVideoOutClose(long long a0) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutClose(static_cast<int>(a0)));
}

int VideoOutElisa_sceVideoOutGetVblankStatus(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetVblankStatus(static_cast<int>(a0), static_cast<Libraries::VideoOut::SceVideoOutVblankStatus*>(a1)));
}

int VideoOutElisa_sceVideoOutGetDeviceCapabilityInfo(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetDeviceCapabilityInfo(static_cast<int>(a0), static_cast<Libraries::VideoOut::SceVideoOutDeviceCapabilityInfo*>(a1)));
}

int VideoOutElisa_sceVideoOutWaitVblank(long long a0) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutWaitVblank(static_cast<int>(a0)));
}

int VideoOutElisa_sceVideoOutGetEventId(void* a0) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetEventId(static_cast<const Libraries::Kernel::OrbisKernelEvent*>(a0)));
}

int VideoOutElisa_sceVideoOutGetEventData(void* a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetEventData(static_cast<const Libraries::Kernel::OrbisKernelEvent*>(a0), static_cast<s64*>(a1)));
}

int VideoOutElisa_sceVideoOutGetEventCount(void* a0) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutGetEventCount(static_cast<const Libraries::Kernel::OrbisKernelEvent*>(a0)));
}

int VideoOutElisa_sceVideoOutColorSettingsSetGamma(void* a0, float a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutColorSettingsSetGamma(static_cast<Libraries::VideoOut::SceVideoOutColorSettings*>(a0), a1));
}

int VideoOutElisa_sceVideoOutAdjustColor(long long a0, void* a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutAdjustColor(static_cast<int>(a0), static_cast<const Libraries::VideoOut::SceVideoOutColorSettings*>(a1)));
}

int VideoOutElisa_sceVideoOutDeleteFlipEvent(long long a0, long long a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutDeleteFlipEvent(static_cast<int>(a0), static_cast<int>(a1)));
}

int VideoOutElisa_sceVideoOutDeleteVblankEvent(long long a0, long long a1) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutDeleteVblankEvent(static_cast<int>(a0), static_cast<int>(a1)));
}

int VideoOutElisa_sceVideoOutModeSetAny_(void* a0, long long a1) {
    Libraries::VideoOut::sceVideoOutModeSetAny_(static_cast<Libraries::VideoOut::Mode*>(a0), static_cast<int>(a1));
    return 0;
}

int VideoOutElisa_sceVideoOutConfigureOutputMode_(long long a0, long long a1, void* a2, void* a3, long long a4, long long a5) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutConfigureOutputMode_(static_cast<int>(a0), static_cast<int>(a1), static_cast<const Libraries::VideoOut::Mode*>(a2), static_cast<const void*>(a3), static_cast<int>(a4), static_cast<int>(a5)));
}

int VideoOutElisa_sceVideoOutSetWindowModeMargins(long long a0, long long a1, long long a2) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutSetWindowModeMargins(static_cast<int>(a0), static_cast<int>(a1), static_cast<int>(a2)));
}

int VideoOutElisa_sceVideoOutSubmitChangeBufferAttribute(long long a0, long long a1, void* a2) {
    return static_cast<int>(Libraries::VideoOut::sceVideoOutSubmitChangeBufferAttribute(static_cast<int>(a0), static_cast<int>(a1), static_cast<const Libraries::VideoOut::BufferAttribute*>(a2)));
}

}
