#include "pad_elisa_bridge.hh"

#include <string.h>

static int g_initialized;
static int g_handle;
static int g_user_id;
static int g_pad_type;
static int g_index;

int PadElisa_scePadInit(void) {
    g_initialized = 1;
    return 0;
}

int PadElisa_scePadOpen(int user_id, int type, int index, const void* param) {
    (void)param;
    if (!g_initialized) {
        return 0x80920005;
    }
    g_handle = 7001;
    g_user_id = user_id;
    g_pad_type = type;
    g_index = index;
    return g_handle;
}

int PadElisa_scePadOpenExt(int user_id, int type, int index, const void* param) {
    return PadElisa_scePadOpen(user_id, type, index, param);
}

int PadElisa_scePadGetHandle(int user_id, int type, int index) {
    if (g_handle != 0 && user_id == g_user_id && type == g_pad_type && index == g_index) {
        return g_handle;
    }
    return 0x80920006;
}

int PadElisa_scePadClose(int handle) {
    if (handle != g_handle) {
        return 0x80920003;
    }
    g_handle = 0;
    return 0;
}

int PadElisa_scePadGetControllerInformation(int handle, struct PadElisaControllerInformation* info) {
    if (handle != g_handle) {
        return 0x80920003;
    }
    memset(info, 0, sizeof(*info));
    info->connectionType = 0;
    info->connectedCount = 1;
    info->connected = true;
    return 0;
}

int PadElisa_scePadGetExtControllerInformation(
    int handle, struct PadElisaExtendedControllerInformation* info) {
    if (handle != g_handle) {
        return 0x80920003;
    }
    memset(info, 0, sizeof(*info));
    info->base.connectedCount = 1;
    return 0;
}

int PadElisa_scePadDeviceClassGetExtendedInformation(
    int handle, struct PadElisaDeviceClassExtendedInformation* info) {
    if (handle != g_handle) {
        return 0x80920003;
    }
    memset(info, 0, sizeof(*info));
    return 0;
}

int PadElisa_scePadGetInfo(struct PadElisaInfo* info) {
    memset(info, 0, sizeof(*info));
    info->unk1 = 1;
    info->pad_handle = 7001;
    info->unk3 = 0x00000101;
    info->colour = 0x102030;
    return 0;
}

int PadElisa_scePadRead(int handle, struct PadElisaData* data, int num) {
    if (handle != g_handle) {
        return 0x80920003;
    }
    memset(data, 0, sizeof(*data));
    data->buttons = 0x1234;
    data->connected = true;
    return num > 0 ? 1 : 0;
}

int PadElisa_scePadReadState(int handle, struct PadElisaData* data) {
    return PadElisa_scePadRead(handle, data, 1) < 0 ? 0x80920003 : 0;
}

int PadElisa_scePadResetLightBar(int handle) {
    return handle == g_handle ? 0 : 0x80920003;
}

int PadElisa_scePadResetOrientation(int handle) {
    return handle == g_handle ? 0 : 0x80920003;
}

int PadElisa_scePadSetLightBar(int handle, const struct PadElisaLightBarParam* param) {
    return handle == g_handle && param != 0 ? 0 : 0x80920004;
}

int PadElisa_scePadSetLightBarForTracker(int handle, const struct PadElisaLightBarParam* param) {
    return PadElisa_scePadSetLightBar(handle, param);
}

int PadElisa_scePadSetVibration(int handle, const struct PadElisaVibrationParam* param) {
    return handle == g_handle && param != 0 ? 0 : 0x80920004;
}

void PadElisa_RegisterLib(void* sym) {
    (void)sym;
}
