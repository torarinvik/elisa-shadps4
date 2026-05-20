#include "core/libraries/voice/voice.h"
#include "core/libraries/voice/voice_elisa_bridge.hh"

#include <cstring>

namespace {
void CopyPortInfo(const Libraries::Voice::OrbisVoicePortInfo& src, VoiceElisaPortInfo* dst) {
    if (dst == nullptr) {
        return;
    }
    std::memset(dst, 0, sizeof(*dst));
    dst->port_type = src.port_type;
    dst->state = src.state;
    dst->edge = src.edge;
    dst->byte_count = src.byte_count;
    dst->frame_size = src.frame_size;
    dst->edge_count = src.edge_count;
    dst->reserved = src.reserved;
}
} // namespace

extern "C" {
int VoiceElisa_sceVoiceConnectIPortToOPort(void) { return Libraries::Voice::sceVoiceConnectIPortToOPort(); }
int VoiceElisa_sceVoiceCreatePort(void) { return Libraries::Voice::sceVoiceCreatePort(); }
int VoiceElisa_sceVoiceDeletePort(void) { return Libraries::Voice::sceVoiceDeletePort(); }
int VoiceElisa_sceVoiceDisconnectIPortFromOPort(void) {
    return Libraries::Voice::sceVoiceDisconnectIPortFromOPort();
}
int VoiceElisa_sceVoiceEnd(void) { return Libraries::Voice::sceVoiceEnd(); }
int VoiceElisa_sceVoiceGetBitRate(uint32_t port_id, uint32_t* bitrate) {
    return Libraries::Voice::sceVoiceGetBitRate(port_id, bitrate);
}
int VoiceElisa_sceVoiceGetMuteFlag(void) { return Libraries::Voice::sceVoiceGetMuteFlag(); }
int VoiceElisa_sceVoiceGetPortAttr(void) { return Libraries::Voice::sceVoiceGetPortAttr(); }
int VoiceElisa_sceVoiceGetPortInfo(uint32_t port_id, VoiceElisaPortInfo* info) {
    Libraries::Voice::OrbisVoicePortInfo native_info{};
    int rc = Libraries::Voice::sceVoiceGetPortInfo(port_id, &native_info);
    if (rc == 0) {
        CopyPortInfo(native_info, info);
    }
    return rc;
}
int VoiceElisa_sceVoiceGetResourceInfo(void) { return Libraries::Voice::sceVoiceGetResourceInfo(); }
int VoiceElisa_sceVoiceGetVolume(void) { return Libraries::Voice::sceVoiceGetVolume(); }
int VoiceElisa_sceVoiceInit(void) { return Libraries::Voice::sceVoiceInit(); }
int VoiceElisa_sceVoiceInitHQ(void) { return Libraries::Voice::sceVoiceInitHQ(); }
int VoiceElisa_sceVoicePausePort(void) { return Libraries::Voice::sceVoicePausePort(); }
int VoiceElisa_sceVoicePausePortAll(void) { return Libraries::Voice::sceVoicePausePortAll(); }
int VoiceElisa_sceVoiceReadFromOPort(void) { return Libraries::Voice::sceVoiceReadFromOPort(); }
int VoiceElisa_sceVoiceResetPort(void) { return Libraries::Voice::sceVoiceResetPort(); }
int VoiceElisa_sceVoiceResumePort(void) { return Libraries::Voice::sceVoiceResumePort(); }
int VoiceElisa_sceVoiceResumePortAll(void) { return Libraries::Voice::sceVoiceResumePortAll(); }
int VoiceElisa_sceVoiceSetBitRate(void) { return Libraries::Voice::sceVoiceSetBitRate(); }
int VoiceElisa_sceVoiceSetMuteFlag(void) { return Libraries::Voice::sceVoiceSetMuteFlag(); }
int VoiceElisa_sceVoiceSetMuteFlagAll(void) { return Libraries::Voice::sceVoiceSetMuteFlagAll(); }
int VoiceElisa_sceVoiceSetThreadsParams(void) { return Libraries::Voice::sceVoiceSetThreadsParams(); }
int VoiceElisa_sceVoiceSetVolume(void) { return Libraries::Voice::sceVoiceSetVolume(); }
int VoiceElisa_sceVoiceStart(void) { return Libraries::Voice::sceVoiceStart(); }
int VoiceElisa_sceVoiceStop(void) { return Libraries::Voice::sceVoiceStop(); }
int VoiceElisa_sceVoiceUpdatePort(void) { return Libraries::Voice::sceVoiceUpdatePort(); }
int VoiceElisa_sceVoiceVADAdjustment(void) { return Libraries::Voice::sceVoiceVADAdjustment(); }
int VoiceElisa_sceVoiceVADSetVersion(void) { return Libraries::Voice::sceVoiceVADSetVersion(); }
int VoiceElisa_sceVoiceWriteToIPort(void) { return Libraries::Voice::sceVoiceWriteToIPort(); }
void VoiceElisa_RegisterLib(void* sym) {
    Libraries::Voice::RegisterLib(static_cast<Core::Loader::SymbolsResolver*>(sym));
}
} // extern "C"
