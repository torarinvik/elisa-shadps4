// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

typedef struct ElisaGuestEntryParams {
    int32_t argc;
    uint32_t padding;
    uintptr_t entry_addr;
    void* argv;
} ElisaGuestEntryParams;

typedef void (*ElisaGuestExitFunc)(int32_t);

static void __attribute__((unused)) elisa_guest_exec_default_exit(int32_t code) {
    (void)code;
    for (;;) {
#if defined(_WIN32)
        Sleep(INFINITE);
#else
        pause();
#endif
    }
}

int32_t ElisaGuestExec_IsSupported(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return 1;
#else
    return 0;
#endif
}

int32_t ElisaGuestExec_MapRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
    }
#if defined(_WIN32)
    DWORD protect = PAGE_NOACCESS;
    if ((prot & 4u) != 0u) {
        protect = ((prot & 2u) != 0u) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else if ((prot & 2u) != 0u) {
        protect = PAGE_READWRITE;
    } else if ((prot & 1u) != 0u) {
        protect = PAGE_READONLY;
    }
    void* result = VirtualAlloc((void*)address, (SIZE_T)size, MEM_RESERVE | MEM_COMMIT, protect);
    return result == (void*)address ? 0 : -1;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
#if defined(MAP_FIXED_NOREPLACE)
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
#else
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
    void* result = mmap((void*)address, (size_t)size, native_prot, flags, -1, 0);
    if (result == MAP_FAILED) {
#if defined(MAP_FIXED_NOREPLACE)
        if (errno == EEXIST) return 0;
#endif
        return -1;
    }
    return result == (void*)address ? 0 : -1;
#endif
}

int32_t ElisaGuestExec_ProtectRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
    }
#if defined(_WIN32)
    DWORD protect = PAGE_NOACCESS;
    DWORD old_protect = 0;
    if ((prot & 4u) != 0u) {
        protect = ((prot & 2u) != 0u) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else if ((prot & 2u) != 0u) {
        protect = PAGE_READWRITE;
    } else if ((prot & 1u) != 0u) {
        protect = PAGE_READONLY;
    }
    return VirtualProtect((void*)address, (SIZE_T)size, protect, &old_protect) ? 0 : -1;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
    return mprotect((void*)address, (size_t)size, native_prot) == 0 ? 0 : -1;
#endif
}

int32_t ElisaGuestExec_WriteBytes(uintptr_t address, const void* data, uint64_t size) {
    if (address == 0 || (data == NULL && size != 0)) {
        return -1;
    }
    if (size != 0) {
        memcpy((void*)address, data, (size_t)size);
    }
    return 0;
}

int32_t ElisaGuestExec_WriteU64(uintptr_t address, uint64_t value) {
    if (address == 0) {
        return -1;
    }
    memcpy((void*)address, &value, sizeof(value));
    return 0;
}

#if defined(__x86_64__) || defined(_M_X64)
#if defined(_WIN32)
int32_t ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    (void)params;
    (void)exit_func;
    return -2;
}
#else
__attribute__((noreturn)) void ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params,
                                                           ElisaGuestExitFunc exit_func) {
    if (exit_func == NULL) {
        exit_func = elisa_guest_exec_default_exit;
    }
    __asm__ volatile(
        "andq $-16, %%rsp\n"
        "subq $8, %%rsp\n"
        "pushq 8(%1)\n"
        "pushq 0(%1)\n"
        "movq %1, %%rdi\n"
        "movq %2, %%rsi\n"
        "jmp *%0\n"
        :
        : "r"((void*)params->entry_addr), "r"(params), "r"(exit_func)
        : "rax", "rsi", "rdi");
    __builtin_unreachable();
}
#endif
#else
int32_t ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    (void)params;
    (void)exit_func;
    return -2;
}
#endif
