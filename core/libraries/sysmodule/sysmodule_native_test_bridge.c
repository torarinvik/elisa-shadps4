#include "sysmodule_elisa_bridge.hh"

static int32_t g_loaded_id;
static int32_t g_handle = 0x2200;

int SysmoduleElisa_sceSysmoduleGetModuleHandleInternal(uint32_t id, int32_t* handle) {
    if ((id & 0x7fffffffU) == 0) {
        return 0x805A1000;
    }
    if ((int32_t)id != g_loaded_id) {
        return 0x805A1001;
    }
    if (handle != 0) {
        *handle = g_handle;
    }
    return 0;
}

int SysmoduleElisa_sceSysmoduleGetModuleInfoForUnwind(uint64_t addr, int32_t flags, void* info) {
    (void)addr;
    (void)flags;
    (void)info;
    return 0;
}

int SysmoduleElisa_sceSysmoduleIsCalledFromSysModule(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleIsCameraPreloaded(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleIsLoaded(uint16_t id) {
    if (id == 0) {
        return 0x805A1000;
    }
    return (int32_t)id == g_loaded_id ? 0 : 0x805A1001;
}

int SysmoduleElisa_sceSysmoduleIsLoadedInternal(uint32_t id) {
    if ((id & 0x7fffffffU) == 0) {
        return 0x805A1000;
    }
    return (int32_t)id == g_loaded_id ? 0 : 0x805A1001;
}

int SysmoduleElisa_sceSysmoduleLoadModule(uint16_t id) {
    if (id == 0) {
        return 0x805A1000;
    }
    g_loaded_id = id;
    return 0;
}

int SysmoduleElisa_sceSysmoduleLoadModuleByNameInternal(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleLoadModuleInternal(uint32_t id) {
    if ((id & 0x7fffffffU) == 0) {
        return 0x805A1000;
    }
    g_loaded_id = (int32_t)id;
    return 0;
}

int SysmoduleElisa_sceSysmoduleLoadModuleInternalWithArg(
    uint32_t id, int32_t argc, const void* argv, uint64_t unk, int32_t* res_out) {
    (void)argc;
    (void)argv;
    if ((id & 0x7fffffffU) == 0 || unk != 0) {
        return 0x805A1000;
    }
    if (res_out != 0) {
        *res_out = 0;
    }
    g_loaded_id = (int32_t)id;
    return 0;
}

int SysmoduleElisa_sceSysmoduleMapLibcForLibkernel(void) {
    return 0;
}

int SysmoduleElisa_sceSysmodulePreloadModuleForLibkernel(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleUnloadModule(uint16_t id) {
    if (id == 0) {
        return 0x805A1000;
    }
    if ((int32_t)id != g_loaded_id) {
        return 0x805A1001;
    }
    g_loaded_id = 0;
    return 0;
}

int SysmoduleElisa_sceSysmoduleUnloadModuleByNameInternal(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleUnloadModuleInternal(void) {
    return 0;
}

int SysmoduleElisa_sceSysmoduleUnloadModuleInternalWithArg(void) {
    return 0;
}

void SysmoduleElisa_RegisterLib(void* sym) {
    (void)sym;
}
