// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/sys_net.h"

extern "C" {

int NetElisa_net_errno() {
    return *Libraries::Kernel::__Error();
}

int NetElisa_sys_socketex(const char* name, int family, int type, int protocol) {
    return Libraries::Net::sys_socketex(name, family, type, protocol);
}

int NetElisa_sys_bind(int s, const void* addr, u32 addrlen) {
    return Libraries::Net::sys_bind(s, static_cast<const Libraries::Net::OrbisNetSockaddr*>(addr),
                                    addrlen);
}

int NetElisa_sys_accept(int s, void* addr, u32* paddrlen) {
    return Libraries::Net::sys_accept(s, static_cast<Libraries::Net::OrbisNetSockaddr*>(addr),
                                      paddrlen);
}

int NetElisa_sys_connect(int s, const void* addr, u32 addrlen) {
    return Libraries::Net::sys_connect(
        s, static_cast<const Libraries::Net::OrbisNetSockaddr*>(addr), addrlen);
}

int NetElisa_sys_getpeername(int s, void* addr, u32* paddrlen) {
    return Libraries::Net::sys_getpeername(s, static_cast<Libraries::Net::OrbisNetSockaddr*>(addr),
                                           paddrlen);
}

int NetElisa_sys_getsockname(int s, void* addr, u32* paddrlen) {
    return Libraries::Net::sys_getsockname(s, static_cast<Libraries::Net::OrbisNetSockaddr*>(addr),
                                           paddrlen);
}

int NetElisa_sys_getsockopt(int s, int level, int optname, void* optval, u32* optlen) {
    return Libraries::Net::sys_getsockopt(s, level, optname, optval, optlen);
}

int NetElisa_sys_listen(int s, int backlog) {
    return Libraries::Net::sys_listen(s, backlog);
}

int NetElisa_sys_setsockopt(int s, int level, int optname, const void* optval, u32 optlen) {
    return Libraries::Net::sys_setsockopt(s, level, optname, optval, optlen);
}

int NetElisa_sys_shutdown(int s, int how) {
    return Libraries::Net::sys_shutdown(s, how);
}

int NetElisa_sys_socketclose(int s) {
    return Libraries::Net::sys_socketclose(s);
}

s64 NetElisa_sys_sendto(int s, const void* buf, u64 len, int flags, const void* addr, u32 addrlen) {
    return Libraries::Net::sys_sendto(
        s, buf, len, flags, static_cast<const Libraries::Net::OrbisNetSockaddr*>(addr), addrlen);
}

s64 NetElisa_sys_sendmsg(int s, const void* msg, int flags) {
    return Libraries::Net::sys_sendmsg(
        s, static_cast<const Libraries::Net::OrbisNetMsghdr*>(msg), flags);
}

s64 NetElisa_sys_recvfrom(int s, void* buf, u64 len, int flags, void* addr, u32* paddrlen) {
    return Libraries::Net::sys_recvfrom(s, buf, len, flags,
                                        static_cast<Libraries::Net::OrbisNetSockaddr*>(addr),
                                        paddrlen);
}

s64 NetElisa_sys_recvmsg(int s, void* msg, int flags) {
    return Libraries::Net::sys_recvmsg(s, static_cast<Libraries::Net::OrbisNetMsghdr*>(msg),
                                       flags);
}

int NetElisa_sys_netabort(int s, int flags) {
    return Libraries::Net::sys_netabort(s, flags);
}

} // extern "C"
