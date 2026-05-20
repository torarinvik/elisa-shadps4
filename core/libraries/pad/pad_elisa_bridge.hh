// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PadElisaControllerInformation {
    uint8_t touch_pad_dummy[16];
    uint8_t stick_dummy[2];
    uint8_t connectionType;
    uint8_t connectedCount;
    bool connected;
    int32_t deviceClass;
    uint8_t reserve[8];
};

struct PadElisaExtendedControllerInformation {
    struct PadElisaControllerInformation base;
    uint16_t padType1;
    uint16_t padType2;
    uint8_t capability;
    uint8_t data[8];
};

struct PadElisaLightBarParam {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t reserve;
};

struct PadElisaVibrationParam {
    uint8_t largeMotor;
    uint8_t smallMotor;
};

struct PadElisaData {
    uint32_t buttons;
    uint8_t left_x;
    uint8_t left_y;
    uint8_t right_x;
    uint8_t right_y;
    uint8_t l2;
    uint8_t r2;
    uint8_t pad0[2];
    uint8_t pad1[80];
    bool connected;
    uint64_t timestamp;
    uint8_t pad2[32];
};

struct PadElisaInfo {
    uint32_t unk1;
    uint32_t unk2;
    uint32_t pad_handle;
    uint32_t unk3;
    uint32_t colour;
};

struct PadElisaDeviceClassExtendedInformation {
    int32_t deviceClass;
    uint8_t reserved[16];
};

int PadElisa_scePadInit(void);
int PadElisa_scePadOpen(int user_id, int type, int index, const void* param);
int PadElisa_scePadOpenExt(int user_id, int type, int index, const void* param);
int PadElisa_scePadGetHandle(int user_id, int type, int index);
int PadElisa_scePadClose(int handle);
int PadElisa_scePadGetControllerInformation(int handle, struct PadElisaControllerInformation* info);
int PadElisa_scePadGetExtControllerInformation(
    int handle, struct PadElisaExtendedControllerInformation* info);
int PadElisa_scePadDeviceClassGetExtendedInformation(
    int handle, struct PadElisaDeviceClassExtendedInformation* info);
int PadElisa_scePadGetInfo(struct PadElisaInfo* info);
int PadElisa_scePadRead(int handle, struct PadElisaData* data, int num);
int PadElisa_scePadReadState(int handle, struct PadElisaData* data);
int PadElisa_scePadResetLightBar(int handle);
int PadElisa_scePadResetOrientation(int handle);
int PadElisa_scePadSetLightBar(int handle, const struct PadElisaLightBarParam* param);
int PadElisa_scePadSetLightBarForTracker(int handle, const struct PadElisaLightBarParam* param);
int PadElisa_scePadSetVibration(int handle, const struct PadElisaVibrationParam* param);
void PadElisa_RegisterLib(void* sym);

#ifdef __cplusplus
} // extern "C"
#endif
