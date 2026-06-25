/* Linux boot bridge: videoout trio + HLE out-pointer variants.
   Safe no-op/zero stubs so these FFI externs resolve to real functions
   instead of NULL on the Linux boot path. Plain C, unmangled symbols. */
#include <stdint.h>

/* videoout (externs in gnmdriver.elisa). Returning port-not-open (false)
   makes gnm_drive_runtime_present skip the flip/vblank submit entirely. */
_Bool videoout_is_port_open(long long handle) { (void)handle; return 0; }
int sceVideoOutSubmitFlip(long long a0, long long a1, long long a2, long long a3) {
    (void)a0; (void)a1; (void)a2; (void)a3; return 0;
}
int sceVideoOutWaitVblank(long long a0) { (void)a0; return 0; }

/* HLE out-pointer event variants (link_name targets in *_ffi_bridge.elisa). */
int UserServiceElisa_GetEvent(unsigned int *out_event_kind, int *out_user_id) {
    (void)out_event_kind; (void)out_user_id; return 0;
}
int SystemServiceElisa_ReceiveEvent(int *out_event_type) { (void)out_event_type; return 0; }
int SystemServiceElisa_GetStatusEventCount(int *out_count) { (void)out_count; return 0; }
int SystemServiceElisa_GetDisplaySafeAreaRatio(float *out_ratio) { (void)out_ratio; return 0; }

/* sysctlbyname: BSD/macOS API absent on Linux. The only caller (guest_exec.elisa
   ge_sysctl_flag) is Apple-gated at runtime, but the extern symbol is still
   referenced at link time on Linux. Stub returns -1 (failure) so callers treat
   the sysctl as unavailable. */
#include <stddef.h>
int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
    (void)name; (void)oldp; (void)oldlenp; (void)newp; (void)newlen;
    return -1;
}
