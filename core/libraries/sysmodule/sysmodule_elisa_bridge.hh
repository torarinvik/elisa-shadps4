#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int SysmoduleElisa_sceSysmoduleGetModuleHandleInternal(uint32_t id, int32_t* handle);
int SysmoduleElisa_sceSysmoduleGetModuleInfoForUnwind(uint64_t addr, int32_t flags, void* info);
int SysmoduleElisa_sceSysmoduleIsCalledFromSysModule(void);
int SysmoduleElisa_sceSysmoduleIsCameraPreloaded(void);
int SysmoduleElisa_sceSysmoduleIsLoaded(uint16_t id);
int SysmoduleElisa_sceSysmoduleIsLoadedInternal(uint32_t id);
int SysmoduleElisa_sceSysmoduleLoadModule(uint16_t id);
int SysmoduleElisa_sceSysmoduleLoadModuleByNameInternal(void);
int SysmoduleElisa_sceSysmoduleLoadModuleInternal(uint32_t id);
int SysmoduleElisa_sceSysmoduleLoadModuleInternalWithArg(uint32_t id, int32_t argc, const void* argv,
                                                         uint64_t unk, int32_t* res_out);
int SysmoduleElisa_sceSysmoduleMapLibcForLibkernel(void);
int SysmoduleElisa_sceSysmodulePreloadModuleForLibkernel(void);
int SysmoduleElisa_sceSysmoduleUnloadModule(uint16_t id);
int SysmoduleElisa_sceSysmoduleUnloadModuleByNameInternal(void);
int SysmoduleElisa_sceSysmoduleUnloadModuleInternal(void);
int SysmoduleElisa_sceSysmoduleUnloadModuleInternalWithArg(void);
void SysmoduleElisa_RegisterLib(void* sym);

#ifdef __cplusplus
}
#endif
