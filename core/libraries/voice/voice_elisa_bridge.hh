#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VoiceElisaPortInfo {
    int32_t port_type;
    int32_t state;
    uint32_t* edge;
    uint32_t byte_count;
    uint32_t frame_size;
    uint16_t edge_count;
    uint16_t reserved;
};

int VoiceElisa_sceVoiceConnectIPortToOPort(void);
int VoiceElisa_sceVoiceCreatePort(void);
int VoiceElisa_sceVoiceDeletePort(void);
int VoiceElisa_sceVoiceDisconnectIPortFromOPort(void);
int VoiceElisa_sceVoiceEnd(void);
int VoiceElisa_sceVoiceGetBitRate(uint32_t port_id, uint32_t* bitrate);
int VoiceElisa_sceVoiceGetMuteFlag(void);
int VoiceElisa_sceVoiceGetPortAttr(void);
int VoiceElisa_sceVoiceGetPortInfo(uint32_t port_id, struct VoiceElisaPortInfo* info);
int VoiceElisa_sceVoiceGetResourceInfo(void);
int VoiceElisa_sceVoiceGetVolume(void);
int VoiceElisa_sceVoiceInit(void);
int VoiceElisa_sceVoiceInitHQ(void);
int VoiceElisa_sceVoicePausePort(void);
int VoiceElisa_sceVoicePausePortAll(void);
int VoiceElisa_sceVoiceReadFromOPort(void);
int VoiceElisa_sceVoiceResetPort(void);
int VoiceElisa_sceVoiceResumePort(void);
int VoiceElisa_sceVoiceResumePortAll(void);
int VoiceElisa_sceVoiceSetBitRate(void);
int VoiceElisa_sceVoiceSetMuteFlag(void);
int VoiceElisa_sceVoiceSetMuteFlagAll(void);
int VoiceElisa_sceVoiceSetThreadsParams(void);
int VoiceElisa_sceVoiceSetVolume(void);
int VoiceElisa_sceVoiceStart(void);
int VoiceElisa_sceVoiceStop(void);
int VoiceElisa_sceVoiceUpdatePort(void);
int VoiceElisa_sceVoiceVADAdjustment(void);
int VoiceElisa_sceVoiceVADSetVersion(void);
int VoiceElisa_sceVoiceWriteToIPort(void);
void VoiceElisa_RegisterLib(void* sym);

#ifdef __cplusplus
}
#endif
