#include "core/libraries/sysmodule/sysmodule.h"
#include "core/libraries/sysmodule/sysmodule_elisa_bridge.hh"

extern "C" {
int SysmoduleElisa_sceSysmoduleGetModuleHandleInternal(uint32_t id, int32_t* handle) {
    return Libraries::SysModule::sceSysmoduleGetModuleHandleInternal(id, handle);
}
int SysmoduleElisa_sceSysmoduleGetModuleInfoForUnwind(uint64_t addr, int32_t flags, void* info) {
    return Libraries::SysModule::sceSysmoduleGetModuleInfoForUnwind(
        addr, flags, static_cast<Libraries::Kernel::OrbisModuleInfoForUnwind*>(info));
}
int SysmoduleElisa_sceSysmoduleIsCalledFromSysModule(void) { return Libraries::SysModule::sceSysmoduleIsCalledFromSysModule(); }
int SysmoduleElisa_sceSysmoduleIsCameraPreloaded(void) { return Libraries::SysModule::sceSysmoduleIsCameraPreloaded(); }
int SysmoduleElisa_sceSysmoduleIsLoaded(uint16_t id) { return Libraries::SysModule::sceSysmoduleIsLoaded(id); }
int SysmoduleElisa_sceSysmoduleIsLoadedInternal(uint32_t id) { return Libraries::SysModule::sceSysmoduleIsLoadedInternal(id); }
int SysmoduleElisa_sceSysmoduleLoadModule(uint16_t id) { return Libraries::SysModule::sceSysmoduleLoadModule(id); }
int SysmoduleElisa_sceSysmoduleLoadModuleByNameInternal(void) { return Libraries::SysModule::sceSysmoduleLoadModuleByNameInternal(); }
int SysmoduleElisa_sceSysmoduleLoadModuleInternal(uint32_t id) { return Libraries::SysModule::sceSysmoduleLoadModuleInternal(id); }
int SysmoduleElisa_sceSysmoduleLoadModuleInternalWithArg(uint32_t id, int32_t argc, const void* argv,
                                                         uint64_t unk, int32_t* res_out) {
    return Libraries::SysModule::sceSysmoduleLoadModuleInternalWithArg(id, argc, argv, unk, res_out);
}
int SysmoduleElisa_sceSysmoduleMapLibcForLibkernel(void) { return Libraries::SysModule::sceSysmoduleMapLibcForLibkernel(); }
int SysmoduleElisa_sceSysmodulePreloadModuleForLibkernel(void) { return Libraries::SysModule::sceSysmodulePreloadModuleForLibkernel(); }
int SysmoduleElisa_sceSysmoduleUnloadModule(uint16_t id) { return Libraries::SysModule::sceSysmoduleUnloadModule(id); }
int SysmoduleElisa_sceSysmoduleUnloadModuleByNameInternal(void) { return Libraries::SysModule::sceSysmoduleUnloadModuleByNameInternal(); }
int SysmoduleElisa_sceSysmoduleUnloadModuleInternal(void) { return Libraries::SysModule::sceSysmoduleUnloadModuleInternal(); }
int SysmoduleElisa_sceSysmoduleUnloadModuleInternalWithArg(void) { return Libraries::SysModule::sceSysmoduleUnloadModuleInternalWithArg(); }
void SysmoduleElisa_RegisterLib(void* sym) {
    Libraries::SysModule::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}
}
