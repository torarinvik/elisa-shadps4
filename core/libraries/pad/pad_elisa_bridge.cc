// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/pad/pad.h"
#include "core/libraries/pad/pad_elisa_bridge.hh"

#include <cstring>

namespace {

void CopyControllerInfo(const Libraries::Pad::OrbisPadControllerInformation& src,
                        PadElisaControllerInformation* dst) {
    if (dst == nullptr) {
        return;
    }
    std::memset(dst, 0, sizeof(*dst));
    dst->connectionType = src.connectionType;
    dst->connectedCount = src.connectedCount;
    dst->connected = src.connected;
    dst->deviceClass = static_cast<int>(src.deviceClass);
}

void CopyExtendedControllerInfo(const Libraries::Pad::OrbisPadExtendedControllerInformation& src,
                                PadElisaExtendedControllerInformation* dst) {
    if (dst == nullptr) {
        return;
    }
    CopyControllerInfo(src.base, &dst->base);
    dst->padType1 = src.padType1;
    dst->padType2 = src.padType2;
    dst->capability = src.capability;
}

void CopyPadData(const Libraries::Pad::OrbisPadData& src, PadElisaData* dst) {
    if (dst == nullptr) {
        return;
    }
    std::memset(dst, 0, sizeof(*dst));
    dst->buttons = static_cast<uint32_t>(src.buttons);
    dst->left_x = src.leftStick.x;
    dst->left_y = src.leftStick.y;
    dst->right_x = src.rightStick.x;
    dst->right_y = src.rightStick.y;
    dst->l2 = src.analogButtons.l2;
    dst->r2 = src.analogButtons.r2;
    dst->connected = src.connected;
    dst->timestamp = src.timestamp;
}

} // namespace

extern "C" {

int PadElisa_scePadInit() {
    return Libraries::Pad::scePadInit();
}

int PadElisa_scePadOpen(int user_id, int type, int index, const void* param) {
    return Libraries::Pad::scePadOpen(
        user_id, type, index,
        static_cast<const Libraries::Pad::OrbisPadOpenParam*>(param));
}

int PadElisa_scePadOpenExt(int user_id, int type, int index, const void* param) {
    return Libraries::Pad::scePadOpenExt(
        user_id, type, index,
        static_cast<const Libraries::Pad::OrbisPadOpenExtParam*>(param));
}

int PadElisa_scePadGetHandle(int user_id, int type, int index) {
    return Libraries::Pad::scePadGetHandle(user_id, type, index);
}

int PadElisa_scePadClose(int handle) {
    return Libraries::Pad::scePadClose(handle);
}

int PadElisa_scePadGetControllerInformation(int handle, PadElisaControllerInformation* info) {
    Libraries::Pad::OrbisPadControllerInformation native_info{};
    const int rc = Libraries::Pad::scePadGetControllerInformation(handle, &native_info);
    if (rc == 0) {
        CopyControllerInfo(native_info, info);
    }
    return rc;
}

int PadElisa_scePadGetExtControllerInformation(int handle,
                                               PadElisaExtendedControllerInformation* info) {
    Libraries::Pad::OrbisPadExtendedControllerInformation native_info{};
    const int rc = Libraries::Pad::scePadGetExtControllerInformation(handle, &native_info);
    if (rc == 0) {
        CopyExtendedControllerInfo(native_info, info);
    }
    return rc;
}

int PadElisa_scePadDeviceClassGetExtendedInformation(
    int handle, PadElisaDeviceClassExtendedInformation* info) {
    Libraries::Pad::OrbisPadDeviceClassExtendedInformation native_info{};
    const int rc = Libraries::Pad::scePadDeviceClassGetExtendedInformation(handle, &native_info);
    if (rc == 0 && info != nullptr) {
        std::memset(info, 0, sizeof(*info));
        info->deviceClass = static_cast<int>(native_info.deviceClass);
    }
    return rc;
}

int PadElisa_scePadGetInfo(PadElisaInfo* info) {
    Libraries::Pad::OrbisPadInfo native_info{};
    const int rc = Libraries::Pad::scePadGetInfo(&native_info);
    if (rc == 0 && info != nullptr) {
        std::memset(info, 0, sizeof(*info));
        info->unk1 = native_info.unk1;
        info->unk2 = native_info.unk2;
        info->pad_handle = native_info.pad_handle;
        info->unk3 = native_info.unk3;
        info->colour = native_info.colour;
    }
    return rc;
}

int PadElisa_scePadRead(int handle, PadElisaData* data, int num) {
    Libraries::Pad::OrbisPadData native_data{};
    const int rc = Libraries::Pad::scePadRead(handle, &native_data, num);
    if (rc > 0) {
        CopyPadData(native_data, data);
    }
    return rc;
}

int PadElisa_scePadReadState(int handle, PadElisaData* data) {
    Libraries::Pad::OrbisPadData native_data{};
    const int rc = Libraries::Pad::scePadReadState(handle, &native_data);
    if (rc == 0) {
        CopyPadData(native_data, data);
    }
    return rc;
}

int PadElisa_scePadResetLightBar(int handle) {
    return Libraries::Pad::scePadResetLightBar(handle);
}

int PadElisa_scePadResetOrientation(int handle) {
    return Libraries::Pad::scePadResetOrientation(handle);
}

int PadElisa_scePadSetLightBar(int handle, const PadElisaLightBarParam* param) {
    if (param == nullptr) {
        return Libraries::Pad::scePadSetLightBar(handle, nullptr);
    }
    const Libraries::Pad::OrbisPadLightBarParam native_param{
        param->r,
        param->g,
        param->b,
        {param->reserve},
    };
    return Libraries::Pad::scePadSetLightBar(
        handle, &native_param);
}

int PadElisa_scePadSetLightBarForTracker(int handle, const PadElisaLightBarParam* param) {
    if (param == nullptr) {
        return Libraries::Pad::scePadSetLightBarForTracker(handle, nullptr);
    }
    const Libraries::Pad::OrbisPadLightBarParam native_param{
        param->r,
        param->g,
        param->b,
        {param->reserve},
    };
    return Libraries::Pad::scePadSetLightBarForTracker(
        handle, &native_param);
}

int PadElisa_scePadSetVibration(int handle, const PadElisaVibrationParam* param) {
    if (param == nullptr) {
        return Libraries::Pad::scePadSetVibration(handle, nullptr);
    }
    const Libraries::Pad::OrbisPadVibrationParam native_param{
        param->largeMotor,
        param->smallMotor,
    };
    return Libraries::Pad::scePadSetVibration(
        handle, &native_param);
}

void PadElisa_RegisterLib(void* sym) {
    Libraries::Pad::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}

} // extern "C"
