#include "voice_elisa_bridge.hh"

#include <string.h>

int VoiceElisa_sceVoiceConnectIPortToOPort(void) { return 0; }
int VoiceElisa_sceVoiceCreatePort(void) { return 0; }
int VoiceElisa_sceVoiceDeletePort(void) { return 0; }
int VoiceElisa_sceVoiceDisconnectIPortFromOPort(void) { return 0; }
int VoiceElisa_sceVoiceEnd(void) { return 0; }
int VoiceElisa_sceVoiceGetBitRate(uint32_t port_id, uint32_t* bitrate) {
    (void)port_id;
    if (bitrate != 0) {
        *bitrate = 48000;
    }
    return 0;
}
int VoiceElisa_sceVoiceGetMuteFlag(void) { return 0; }
int VoiceElisa_sceVoiceGetPortAttr(void) { return 0; }
int VoiceElisa_sceVoiceGetPortInfo(uint32_t port_id, struct VoiceElisaPortInfo* info) {
    (void)port_id;
    if (info != 0) {
        memset(info, 0, sizeof(*info));
        info->frame_size = 1;
    }
    return 0;
}
int VoiceElisa_sceVoiceGetResourceInfo(void) { return 0; }
int VoiceElisa_sceVoiceGetVolume(void) { return 0; }
int VoiceElisa_sceVoiceInit(void) { return 0; }
int VoiceElisa_sceVoiceInitHQ(void) { return 0; }
int VoiceElisa_sceVoicePausePort(void) { return 0; }
int VoiceElisa_sceVoicePausePortAll(void) { return 0; }
int VoiceElisa_sceVoiceReadFromOPort(void) { return 0; }
int VoiceElisa_sceVoiceResetPort(void) { return 0; }
int VoiceElisa_sceVoiceResumePort(void) { return 0; }
int VoiceElisa_sceVoiceResumePortAll(void) { return 0; }
int VoiceElisa_sceVoiceSetBitRate(void) { return 0; }
int VoiceElisa_sceVoiceSetMuteFlag(void) { return 0; }
int VoiceElisa_sceVoiceSetMuteFlagAll(void) { return 0; }
int VoiceElisa_sceVoiceSetThreadsParams(void) { return 0; }
int VoiceElisa_sceVoiceSetVolume(void) { return 0; }
int VoiceElisa_sceVoiceStart(void) { return 0; }
int VoiceElisa_sceVoiceStop(void) { return 0; }
int VoiceElisa_sceVoiceUpdatePort(void) { return 0; }
int VoiceElisa_sceVoiceVADAdjustment(void) { return 0; }
int VoiceElisa_sceVoiceVADSetVersion(void) { return 0; }
int VoiceElisa_sceVoiceWriteToIPort(void) { return 0; }
void VoiceElisa_RegisterLib(void* sym) { (void)sym; }
