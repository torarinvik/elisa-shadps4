// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/pad/pad.h"

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

int PadElisa_scePadGetControllerInformation(int handle, void* info) {
    return Libraries::Pad::scePadGetControllerInformation(
        handle, static_cast<Libraries::Pad::OrbisPadControllerInformation*>(info));
}

int PadElisa_scePadGetExtControllerInformation(int handle, void* info) {
    return Libraries::Pad::scePadGetExtControllerInformation(
        handle, static_cast<Libraries::Pad::OrbisPadExtendedControllerInformation*>(info));
}

int PadElisa_scePadDeviceClassGetExtendedInformation(int handle, void* info) {
    return Libraries::Pad::scePadDeviceClassGetExtendedInformation(
        handle, static_cast<Libraries::Pad::OrbisPadDeviceClassExtendedInformation*>(info));
}

int PadElisa_scePadGetInfo(void* info) {
    return Libraries::Pad::scePadGetInfo(static_cast<Libraries::Pad::OrbisPadInfo*>(info));
}

int PadElisa_scePadRead(int handle, void* data, int num) {
    return Libraries::Pad::scePadRead(handle, static_cast<Libraries::Pad::OrbisPadData*>(data),
                                     num);
}

int PadElisa_scePadReadState(int handle, void* data) {
    return Libraries::Pad::scePadReadState(handle,
                                          static_cast<Libraries::Pad::OrbisPadData*>(data));
}

int PadElisa_scePadResetLightBar(int handle) {
    return Libraries::Pad::scePadResetLightBar(handle);
}

int PadElisa_scePadResetOrientation(int handle) {
    return Libraries::Pad::scePadResetOrientation(handle);
}

int PadElisa_scePadSetLightBar(int handle, const void* param) {
    return Libraries::Pad::scePadSetLightBar(
        handle, static_cast<const Libraries::Pad::OrbisPadLightBarParam*>(param));
}

int PadElisa_scePadSetLightBarForTracker(int handle, const void* param) {
    return Libraries::Pad::scePadSetLightBarForTracker(
        handle, static_cast<const Libraries::Pad::OrbisPadLightBarParam*>(param));
}

int PadElisa_scePadSetVibration(int handle, const void* param) {
    return Libraries::Pad::scePadSetVibration(
        handle, static_cast<const Libraries::Pad::OrbisPadVibrationParam*>(param));
}

void PadElisa_RegisterLib(void* sym) {
    Libraries::Pad::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}

} // extern "C"
